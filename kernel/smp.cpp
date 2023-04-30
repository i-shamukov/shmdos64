/*
   smp.cpp
   Kernel SMP low-level initialization
   SHM DOS64
   Copyright (c) 2023, Ilya Shamukov, ilya.shamukov@gmail.com
   
   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the Free
   Software Foundation; either version 2 of the License, or (at your option) 
   any later version.
   
   This program is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or 
   FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for 
   more details.
   
   You should have received a copy of the GNU General Public License along with 
   this program; if not, write to the Free Software Foundation, Inc., 59 Temple 
   Place, Suite 330, Boston, MA 02111-1307 USA
*/

#include <cpu.h>
#include <kclib.h>
#include <conout.h>
#include <VirtualMemoryManager.h>
#include <kthread.h>
#include "paging.h"
#include "SmpBoot.h"
#include "AcpiTables.h"
#include "gdt.h"
#include "idt.h"
#include "TaskManager.h"
#include "Task.h"
#include "smp.h"

static bool g_smpInit = false;

static void cpuInitProc(unsigned int cpuId, void* localCpuData, void* localCpuSpinsData, const SystemTaskSegmentState* tss, kthread* firstThread, const kvector<CpuMtrrItem>& mtrr)
{
	cpuSetDefaultControlRegisters();
	SystemSMP::initCpu(cpuId, localCpuData, localCpuSpinsData);
	SystemGDT::install(cpuId, tss);
	PagingManager64::system().initCpu(cpuId);
	SystemIDT::install();
	cpuInitFpu();
	LocalApic::system().initCurrentCpu();
	TaskManager::system()->initCurrentCpu(firstThread);
	cpuLoadMtrr(mtrr);
	cpuEnableInterrupts();
	TaskManager::terminateCurrentTask();
}

INTERRUPT_HANDLER(cpuStopHandler, "")
{
	(void)state;
	cpuStop();
}

bool SystemSMP::isInit()
{
	return g_smpInit;
}

void SystemSMP::initBootCpu()
{
	static uint8_t bootCpuLocalData[LOCAL_CPU_DATA_SIZE] alignas(16) = {};
	static uint8_t bootCpuLocalSpinData[PAGE_SIZE] alignas(16) = {};
	initCpu(BOOT_CPU_ID, bootCpuLocalData, bootCpuLocalSpinData);
}

void SystemSMP::initCpu(unsigned int cpuId, void* localCpuData, void* localCpuSpinsData)
{
	uint8_t* curCpuData = static_cast<uint8_t*>(localCpuData);
	kmemcpy(curCpuData + LOCAL_CPU_ID, &cpuId, sizeof (cpuId));
	const PagingManager64 * const pagingMgr = &PagingManager64::system();
	kmemcpy(curCpuData + LOCAL_CPU_PAGING_MGR, &pagingMgr, sizeof(pagingMgr));
	kmemset(curCpuData + LOCAL_CPU_TS_FLAG, 0, sizeof(uintptr_t));
	kmemcpy(curCpuData + LOCAL_CPU_SPIN_DATA, &localCpuSpinsData, sizeof(localCpuSpinsData));
	cpuWriteMSR(CPU_MSR_FS_BASE, reinterpret_cast<uintptr_t>(curCpuData));
	initSpinDataOnCurrentCpu();
}

void SystemSMP::init()
{
	const kvector<LocalApic::ApicCpuId>& apicIds = AcpiTables::instance().cpuApicIds();
	const LocalApic::ApicCpuId bootCpuApicId = LocalApic::getCpuId();
	unsigned int cpuCnt = BOOT_CPU_ID + 1;
	const uintptr_t loaderStart = reinterpret_cast<uintptr_t>(&smpBootLoaderStart);
	const uintptr_t loaderEnd = reinterpret_cast<uintptr_t>(&smpBootLoaderEnd);
	const size_t loaderSize = loaderEnd - loaderStart;
	if (loaderSize > SMP_BOOT_CODE_MAX_SIZE)
	{
		println(L"Too long SMP bootloader code");
		return;
	}
	
	SystemIDT::setHandler(CPU_STOP_VECTOR, &cpuStopHandler, true);
	
	PagingManager64::initSmp();
	PagingManager64& pagingMgr = PagingManager64::system();
	const size_t bootMemorySize = SMP_BOOT_MEMORY_END - SMP_BOOT_MEMORY_START;
	pagingMgr.mapPages(SMP_BOOT_MEMORY_START, SMP_BOOT_MEMORY_START, bootMemorySize, PAGE_FLAG_WRITE | PAGE_FLAG_PRESENT);
	kmemcpy(reinterpret_cast<void*>(SMP_BOOT_CODE_BASE), reinterpret_cast<void*>(loaderStart), loaderSize);
	SystemGDT::storeBootPart(reinterpret_cast<void*>(SMP_BOOT_GDT_TABLE_ADDR), reinterpret_cast<void*>(SMP_BOOT_GDT_POINTER_ADDR));
	pagingMgr.storeRootTable(reinterpret_cast<void*>(SMP_BOOT_PAGE_TABLE_ROOT));
	LocalApic& apic = LocalApic::system();
	VirtualMemoryManager& vmm = VirtualMemoryManager::system();
	const kvector<CpuMtrrItem>& mtrr = cpuStoreMtrr();
	g_smpInit = true;
	for (const LocalApic::ApicCpuId apicId : apicIds)
	{
		if (bootCpuApicId == apicId)
			continue;
		
		print(L"Initializing CPU[", cpuCnt, L", APIC_ID = ", apicId, L"]... ");
		SystemTaskSegmentState* tss = new SystemTaskSegmentState();
		kmemset(tss, 0, sizeof(*tss));
		void* localCpuData = vmm.alloc(LOCAL_CPU_DATA_SIZE, VMM_READWRITE | VMM_COMMIT);
		kmemset(localCpuData, 0, LOCAL_CPU_DATA_SIZE);
		void* localCpuSpinsData = vmm.alloc(PAGE_SIZE, VMM_READWRITE | VMM_COMMIT);
		kmemset(localCpuSpinsData, 0, PAGE_SIZE);
		kthread th(std::bind(&cpuInitProc, cpuCnt, localCpuData, localCpuSpinsData, tss, &th, std::ref(mtrr)), false);
		Task* task = TaskManager::extractTask(th);
		const InterruptVolatileState& vstate = reinterpret_cast<InterruptFullState*>(task->m_stackTop)->m_volatile;
		const uintptr_t rip = vstate.m_frame.m_rip;
		kmemcpy(reinterpret_cast<void*>(SMP_BOOT_ENTRY_POINT), &rip, sizeof(rip));
		kmemcpy(reinterpret_cast<void*>(SMP_BOOT_STACK_TOP), &task->m_stackTop, sizeof(task->m_stackTop));
		kmemcpy(reinterpret_cast<void*>(SMP_BOOT_ENTRY_ARG), &vstate.m_regs.m_rcx, sizeof(vstate.m_regs.m_rcx));
		apic.runCpu(apicId, SMP_BOOT_CODE_BASE >> 4);
		th.join();
		println("OK");
		++cpuCnt;
	}
	pagingMgr.mapPages(SMP_BOOT_MEMORY_START, 0, bootMemorySize, 0);
}
