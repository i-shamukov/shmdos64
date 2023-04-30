/*
   paging.cpp
   Kernel memory 4-level paging manager
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
#include <kernel_params.h>
#include <vmem_utils.h>
#include <kclib.h>
#include <kalgorithm.h>
#include <VirtualMemoryManager.h>
#include "phmem.h"
#include "panic.h"
#include "paging.h"
#include "smp.h"
#include "LocalApic.h"
#include "idt.h"

#include <conout.h>

enum : uint64_t
{
	CPU_VIRTUAL_ADDRESS_MASK = 0xFFFFFFFFFFFF
};

struct TlbShotdownItem
{
	uintptr_t m_addr;
	size_t m_size;
};

static const size_t g_defaultTlbRingBufferSize = 64;
struct CpuTlbShootdownTask
{
	QueuedSpinLockSm m_spin;
	TlbShotdownItem m_items[g_defaultTlbRingBufferSize];
	size_t m_readIndex = 0;
	size_t m_writeIndex = 0;
	bool m_needFull = false;
};

static CpuTlbShootdownTask* g_cpuTlbShootdownTask[MAX_CPU];
static std::atomic<PagingManager64*> g_pagingManagers[MAX_CPU];

static void localCpuFlushTlb(uintptr_t virtualBase, size_t size)
{
	if (size <= PAGE_SIZE)
		cpuFlushTLB(virtualBase);
	else if (size >= CPU_UPDATE_TLB_LIMIT)
		cpuFullFlushTLB(virtualBase);
	else
	{
		const uintptr_t end = virtualBase + size;
		for (uintptr_t addr = virtualBase; addr < end; addr += PAGE_SIZE)
			cpuFlushTLB(addr);
	}
}

INTERRUPT_HANDLER(tlbShutdownHandler, "")
{
	(void)state;
	CpuTlbShootdownTask* tlbTask = static_cast<CpuTlbShootdownTask*>(cpuGetLocalPtr(LOCAL_CPU_TLB_TASK));
	{
		kunique_lock lock(tlbTask->m_spin);
		if (tlbTask->m_needFull)
		{
			tlbTask->m_needFull = false;
			lock.unlock();
			cpuFullFlushTLB(0);
		}
		else
		{
			while (tlbTask->m_readIndex != tlbTask->m_writeIndex)
			{
				const TlbShotdownItem& item = tlbTask->m_items[tlbTask->m_readIndex++ % g_defaultTlbRingBufferSize];
				lock.unlock();
				const uintptr_t end = item.m_addr + item.m_size;
				for (uintptr_t addr = item.m_addr; addr < end; addr += PAGE_SIZE)
					cpuFlushTLB(addr);
				lock.lock();
			}
		}
	}
	cpuFastEio();
}

static uint64_t* accessToChildDir(uint64_t* dir, int index, uint64_t dirFlags)
{
	uint64_t& entry = dir[index];
	if (entry == 0)
	{
		if (dirFlags == 0)
			PANIC(L"Creating page subdir with clear flags");

		entry = RamAllocator::getInstance().allocPage(true) | dirFlags;
	}
	return physToVirtualInt<uint64_t>(entry & (CPU_PHYSICAL_ADDRESS_MASK & ~PAGE_MASK));
}

PagingManager64::PagingManager64(bool system)
	: m_system(system)
{
	if (system)
	{
		m_cr3 = cpuGetCR3();
		m_dir256tb = physToVirtualInt<uint64_t>(m_cr3 & (CPU_PHYSICAL_ADDRESS_MASK & ~PAGE_MASK));
	}
	else
	{
		PANIC(L"User mode is not implemented");
	}
}

PagingManager64::~PagingManager64()
{
	PANIC(L"not implemented");
}

template<typename Runnable>
void PagingManager64::processPagesLevel(unsigned int level, uint64_t* dir, uint64_t pageStart, uint64_t pageEnd, uint64_t dirFlags, Runnable callback, bool& needShootdown)
{
	if (level == Levels)
	{
		while (pageStart <= pageEnd)
		{
			uint64_t& value = dir[pageStart++];
			if ((value & PAGE_FLAG_PRESENT) != 0)
				needShootdown = true;
			callback(value);
		}
		return;
	}

	const uint64_t shift = (Levels - level) * 9;
	const uint64_t size = (1LL << shift);
	const uint64_t mask = size - 1;
	const uint64_t fistLimit = (pageStart + size) & ~mask;
	uint64_t* child = accessToChildDir(dir, static_cast<int>(pageStart >> shift), dirFlags);
	if (pageEnd < fistLimit)
	{
		processPagesLevel(level + 1, child, pageStart & mask, pageEnd & mask, dirFlags, callback, needShootdown);
		return;
	}
	else
	{
		processPagesLevel(level + 1, child, pageStart & mask, mask, dirFlags, callback, needShootdown);
		const int lastPage = static_cast<int>((pageEnd + 1) >> shift);
		for (int index = static_cast<int>(fistLimit >> shift); index < lastPage; index++)
		{
			child = accessToChildDir(dir, index, dirFlags);
			processPagesLevel(level + 1, child, 0, mask, dirFlags, callback, needShootdown);
		}
		if (((pageEnd + 1) & mask) != 0)
		{
			child = accessToChildDir(dir, lastPage, dirFlags);
			processPagesLevel(level + 1, child, 0, pageEnd & mask, dirFlags, callback, needShootdown);
		}
	}
}

template<typename Runnable>
void PagingManager64::processPages(uintptr_t virtualBase, size_t size, uint64_t dirFlags, Runnable callback)
{
	if (size == 0)
		return;
	
	if (!m_system && virtualBase >= KERNEL_VIRTUAL_BASE)
	{
		PagingManager64* system = &PagingManager64::system();
		if (this != system)
			return system->processPages(virtualBase, size, dirFlags, callback);
	}

	uintptr_t normalizeVirtualBase = virtualBase & CPU_VIRTUAL_ADDRESS_MASK;
	bool needShutdown = false;
	{
		klock_guard lock(m_spin);
		processPagesLevel(
			1,
			m_dir256tb,
			(normalizeVirtualBase >> PAGE_SHIFT),
			(((size + normalizeVirtualBase + PAGE_MASK) >> PAGE_SHIFT) - 1),
			dirFlags,
			callback,
			needShutdown);
	}
	flushPagesTlb(virtualBase, size, needShutdown);
}

template<typename Runnable>
void PagingManager64::processPage(uintptr_t virtualBase, uint64_t dirFlags, Runnable callback)
{
	if (!m_system && virtualBase >= KERNEL_VIRTUAL_BASE)
	{
		PagingManager64* system = &PagingManager64::system();
		if (this != system)
			return system->processPage(virtualBase, dirFlags, callback);
	}

	uint64_t* dir = m_dir256tb;
	bool needShutdown;
	{
		klock_guard lock(m_spin);
		for (uint64_t shift = PAGE_SHIFT + (Levels - 1) * 9; shift > PAGE_SHIFT; shift -= 9)
			dir = accessToChildDir(dir, (virtualBase >> shift) & 0x1FF, dirFlags);
		uint64_t& value = dir[(virtualBase >> PAGE_SHIFT) & 0x1FF];
		needShutdown = ((value & PAGE_FLAG_PRESENT) != 0);
		callback(value);
	}
	flushPagesTlb(virtualBase, PAGE_SIZE, needShutdown);
}

void PagingManager64::mapPages(uintptr_t virtualBase, uintptr_t physBase, size_t size, uint64_t pageFlags)
{
	processPages(
		virtualBase,
		size,
		((pageFlags & PAGE_FLAG_USER) | PAGE_FLAG_PRESENT | PAGE_FLAG_WRITE),
		[&physBase, pageFlags](uint64_t & pageEntry) {
			pageEntry = physBase | pageFlags;
			physBase += PAGE_SIZE;
		});
}

void PagingManager64::setPagesFlags(uintptr_t virtualBase, size_t size, uint64_t mask, uint64_t pageFlags)
{
	processPages(
		virtualBase,
		size,
		((pageFlags & PAGE_FLAG_USER) | PAGE_FLAG_PRESENT | PAGE_FLAG_WRITE),
		[mask, pageFlags](uint64_t & pageEntry) {
			pageEntry &= ~mask;
			pageEntry |= pageFlags;
		});
}

void PagingManager64::freeRamPages(uintptr_t virtualBase, size_t size, bool virtualAllocated)
{
	RamAllocator& allocator = RamAllocator::getInstance();
	processPages(
		virtualBase,
		size,
		0,
		[&allocator, virtualAllocated](uint64_t & pageEntry) {
			if ((pageEntry & PAGE_FLAG_PRESENT) != 0)
			{
				const uintptr_t addr = pageEntry & (CPU_PHYSICAL_ADDRESS_MASK & ~PAGE_MASK);
				if (addr != 0)
				{
					allocator.freePage(addr);
					pageEntry &= PAGE_MASK ^ PAGE_FLAG_PRESENT;
					if (virtualAllocated)
						pageEntry |= PAGE_FLAG_ALLOCATED;
				}
			}
		});
}

void PagingManager64::freeRamPage(uintptr_t virtualBase, bool virtualAllocated)
{
	processPage(
		virtualBase,
		0,
		[virtualAllocated](uint64_t & pageEntry) {
			if ((pageEntry & PAGE_FLAG_PRESENT) != 0)
			{
				const uintptr_t addr = pageEntry & (CPU_PHYSICAL_ADDRESS_MASK & ~PAGE_MASK);
				if (addr != 0)
				{
					RamAllocator::getInstance().freePage(addr);
					pageEntry &= PAGE_MASK ^ PAGE_FLAG_PRESENT;
					if (virtualAllocated)
						pageEntry |= PAGE_FLAG_ALLOCATED;
				}
			}
		});
}

void PagingManager64::processPages(uintptr_t virtualBase, size_t size, PageProc callback, uint64_t dirFlags)
{
	processPages<PageProc>(virtualBase, size, dirFlags, callback);
}

void PagingManager64::mapPage(uintptr_t virtualBase, uintptr_t physBase, uint64_t pageFlags)
{
	const auto entryValue = physBase | pageFlags;
	processPage(
		virtualBase,
		((pageFlags & PAGE_FLAG_USER) | PAGE_FLAG_PRESENT | PAGE_FLAG_WRITE),
		[entryValue](uint64_t & pageEntry) {
			pageEntry = entryValue;
		});
}

void PagingManager64::processPage(uintptr_t virtualBase, PageProc callback, uint64_t dirFlags)
{
	processPage<PageProc>(virtualBase, dirFlags, callback);
}

PagingManager64& PagingManager64::system()
{
	static PagingManager64 mgr(true);
	return mgr;
}

void PagingManager64::unmapBootPages()
{
	uint64_t* dir512g = physToVirtualInt<uint64_t>(cpuGetCR3() & (CPU_PHYSICAL_ADDRESS_MASK & ~PAGE_MASK));
	kmemset(dir512g, 0, PAGE_SIZE / 2);
	cpuFullFlushTLB(0);
}

bool PagingManager64::onPageFault(uintptr_t addr, uint64_t errorCode)
{

	enum : uint64_t
	{
		ecPageProtection = (1 << 0)
	};
	if ((errorCode & ecPageProtection) != 0)
		return false;

	uint64_t* dir = m_dir256tb;
	klock_guard lock(m_spin);
	for (uint64_t shift = PAGE_SHIFT + (Levels - 1) * 9; shift > PAGE_SHIFT; shift -= 9)
	{
		const uint64_t dirValue = dir[(addr >> shift) & 0x1FF];
		const uint64_t phys = dirValue & (CPU_PHYSICAL_ADDRESS_MASK & ~PAGE_MASK);
		if (phys == 0)
			return false;

		if ((dirValue & PAGE_FLAG_SIZE) != 0)
			PANIC(L"Fault in large page");

		dir = physToVirtualInt<uint64_t>(phys);
	}
	uint64_t& pageValue = dir[(addr >> PAGE_SHIFT) & 0x1FF];
	if ((pageValue & PAGE_FLAG_PRESENT) != 0)	
	{
		// allocated on other CPU
		cpuFlushTLB(addr);
		return true;
	}
	else if ((pageValue & PAGE_FLAG_ALLOCATED) == 0)
	{
		return false;
	}

	const uint64_t physPage = RamAllocator::getInstance().allocPage((pageValue & PAGE_FLAG_MEMZERO) != 0);
	pageValue &= PAGE_MASK ^ PAGE_FLAG_ALLOCATED;
	pageValue |= static_cast<uint64_t>(physPage) | PAGE_FLAG_PRESENT;
	cpuFlushTLB(addr);
	return true;
}

PagingManager64* PagingManager64::current()
{
	return static_cast<PagingManager64*>(cpuGetLocalPtr(LOCAL_CPU_PAGING_MGR));
}

void PagingManager64::setCurrent(PagingManager64* pagingMgr)
{
	cpuSetLocalPtr(LOCAL_CPU_PAGING_MGR, pagingMgr);
	g_pagingManagers[cpuCurrentId()].store(pagingMgr, std::memory_order_relaxed);
	cpuSetCR3(pagingMgr->m_cr3);
}

void PagingManager64::storeRootTable(void* table)
{
	kmemcpy(table, m_dir256tb, PAGE_SIZE);
}

void PagingManager64::initSmp()
{
	VirtualMemoryManager& vmm = VirtualMemoryManager::system();
	const unsigned int numCpu = cpuLogicalCount();
	for (unsigned int cpuId = 0; cpuId < numCpu; ++cpuId)
	{
		void* task = vmm.alloc(sizeof(CpuTlbShootdownTask), VMM_READWRITE | VMM_COMMIT); 
		new (task) CpuTlbShootdownTask();
		g_cpuTlbShootdownTask[cpuId] = static_cast<CpuTlbShootdownTask*>(task);
	}
	
	g_pagingManagers[BOOT_CPU_ID].store(&PagingManager64::system(), std::memory_order_relaxed);
	cpuSetLocalPtr(LOCAL_CPU_TLB_TASK, g_cpuTlbShootdownTask[BOOT_CPU_ID]);
	SystemIDT::setHandler(CPU_TLB_SHOOTDOWN_VECTOR, &tlbShutdownHandler, true);
}

void PagingManager64::initCpu(unsigned int cpuId)
{
	cpuSetCR3(m_cr3);
	CpuTlbShootdownTask* tlbTask = g_cpuTlbShootdownTask[cpuId];
	cpuSetLocalPtr(LOCAL_CPU_TLB_TASK, tlbTask);
	tlbTask->m_readIndex = 0;
	tlbTask->m_writeIndex = 0;
	tlbTask->m_needFull = false;
	g_pagingManagers[cpuId].store(this, std::memory_order_relaxed);
}

void PagingManager64::flushPagesTlb(uintptr_t virtualBase, size_t size, bool needShootdown)
{
	localCpuFlushTlb(virtualBase, size);
	if (needShootdown && SystemSMP::isInit())
	{
		const unsigned int curCpuId = cpuCurrentId();
		const unsigned int numCpu = cpuLogicalCount();
		const TlbShotdownItem item{virtualBase, size};
		LocalApic& apic = LocalApic::system();
		const bool userMem = (virtualBase < KERNEL_VIRTUAL_BASE);
		const bool needFull = size >= CPU_UPDATE_TLB_LIMIT;
		unsigned int cpuList[MAX_CPU];
		unsigned int cpuListCount = 0;
		for (unsigned int cpuId = 0; cpuId < numCpu; ++cpuId)
		{
			if (cpuId == curCpuId)
				continue;
			
			if (userMem && (g_pagingManagers[cpuId].load(std::memory_order_acquire) != this))
				continue;
			
			CpuTlbShootdownTask* tlbTask = g_cpuTlbShootdownTask[cpuId];
			{
				CpuInterruptLockSave intLock;
				klock_guard lock(tlbTask->m_spin);
				if (tlbTask->m_needFull)
					continue;
				
				const size_t oldItems = (tlbTask->m_writeIndex - tlbTask->m_readIndex) % g_defaultTlbRingBufferSize;
				if ((oldItems == (g_defaultTlbRingBufferSize - 1)) || needFull)
				{
					tlbTask->m_writeIndex = 0;
					tlbTask->m_readIndex = 0;
					tlbTask->m_needFull = true;
				}
				else
				{
					tlbTask->m_items[tlbTask->m_writeIndex++ % g_defaultTlbRingBufferSize] = item;
				}
				if (oldItems == 0)
				{
					cpuList[cpuListCount++] = cpuId;
				}
			}
		}
		
		if ( (cpuListCount + 1) != numCpu)
		{
			for (unsigned int idx = 0; idx < cpuListCount; ++idx)
				apic.sendIpi(LocalApic::systemCpuIdToApic(cpuList[idx]), CPU_TLB_SHOOTDOWN_VECTOR);
		}
		else
		{
			apic.sendBroadcastIpi(CPU_TLB_SHOOTDOWN_VECTOR);
		}
	}
}