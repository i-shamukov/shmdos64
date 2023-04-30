/*
   cpu.cpp
   CPU routines for kernel
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

#include <kernel_params.h>
#include <cpu.h>
#include <conout.h>
#include <cpuid.h>
#include "smp.h"
#include "AcpiTables.h"
#include "panic.h"


#define CPU_INT_TO_STR_HELPER(value) #value
#define CPU_INT_TO_STR(value) CPU_INT_TO_STR_HELPER(value)
asm volatile(
	".globl cpuFastEio\n"
	"cpuFastEio:\n"
	"push RBX\n"
	"mov RBX, FS:[" CPU_INT_TO_STR(LOCAL_CPU_APIC_EOI_ADDR_MACRO) "]\n"
	"mov dword ptr [RBX], 0\n"
	"pop RBX\n"
	"ret"
);

void cpuStop()
{
	if (SystemSMP::isInit())
	{
		LocalApic::system().sendBroadcastIpi(CPU_STOP_VECTOR);
	}
	cpuDisableInterrupts();
	cpuHalt();
}

unsigned int cpuCurrentId()
{
	return static_cast<unsigned int>(cpuGetLocalData(LOCAL_CPU_ID));
}

void cpuLoadCS(uint16_t selector)
{
	asm volatile("push RAX\n"
			"lea RAX, [RIP + __cpuLoadCS_end]\n"
			"push RAX\n"
			".byte 0x48\n"
			"retf\n"
			"__cpuLoadCS_end:\n"::"a"(selector));

}

uint64_t cpuGetFlagsRegister()
{
	uint64_t result;
	asm volatile("pushfq\n"
			"popq %0\n" : "=r"(result) :);
	return result;
}

void cpuFullFlushTLB(uintptr_t addr)
{
	if ((addr == 0) || (addr >= KERNEL_VIRTUAL_BASE))
	{
		const uint64_t cr4 = cpuGetCR4();
		cpuSetCR4(cr4 ^ CR4_PAGE_GLOBAL_ENABLE);
		cpuSetCR4(cr4);
	}
	else
	{
		cpuSetCR3(cpuGetCR3());
	}
}

uintptr_t cpuMaxPhysAddr()
{
	static const uintptr_t result = [] {
		uint32_t eax = CPUID_MAX_ADDR;
		uint32_t ebx;
		uint32_t ecx;
		uint32_t edx;
		cpuCpuid(eax, ebx, ecx, edx);
		return (1ULL << kmax(static_cast<uint64_t>(eax & 0xFF), 36ULL)) - 1;
	}();
	return result;
}

unsigned int cpuLogicalCount()
{
	static const unsigned int count = [] {
		const size_t apics = AcpiTables::instance().cpuApicIds().size();
		return ((apics > 0) ? apics : 1);
	}();
	return count;
}

void cpuSetTsFlag()
{
	if (cpuGetLocalData(LOCAL_CPU_TS_FLAG) == 0)
	{
		static const uintptr_t CR0_TS = 0x08;
		cpuSetCR0(cpuGetCR0() | CR0_TS);
		cpuSetLocalData(LOCAL_CPU_TS_FLAG, 1);
	}
}

void cpuClearTsFlag()
{
	if (cpuGetLocalData(LOCAL_CPU_TS_FLAG) != 0)
	{
		asm volatile("clts");
		cpuSetLocalData(LOCAL_CPU_TS_FLAG, 0);
	}
}

kvector<CpuMtrrItem> cpuStoreMtrr()
{
	kvector<CpuMtrrItem> result;
	const int vcnt = cpuReadMSR(CPU_MSR_IA32_MTRRCAP) & 0xFF;
	result.reserve(vcnt);
	for (int idx = 0; idx < vcnt; ++idx)
	{
		CpuMtrrItem item;
		item.m_addr = cpuReadMSR(CPU_MSR_IA32_MTRR_PHYSBASE0 + idx * 2);
		item.m_mask = cpuReadMSR(CPU_MSR_IA32_MTRR_PHYSMASK0 + idx * 2);
		result.push_back(item);
	}
	return result;
}

void cpuLoadMtrr(const kvector<CpuMtrrItem>& mtrr)
{
	if (mtrr.empty())
		return;
	
	const uint64_t cr0 = cpuGetCR0();
	cpuSetCR0(cr0 | CR0_CACHE_DISABLE);
	cpuInvalidateCache();
	for (size_t idx = 0; idx < mtrr.size(); ++idx)
	{
		const CpuMtrrItem& item = mtrr[idx];
		cpuWriteMSR(CPU_MSR_IA32_MTRR_PHYSBASE0 + idx * 2, item.m_addr);
		cpuWriteMSR(CPU_MSR_IA32_MTRR_PHYSMASK0 + idx * 2, item.m_mask);
	}
	cpuInvalidateCache();
	cpuFullFlushTLB(0);
	cpuSetCR0(cr0);
}
