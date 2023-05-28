/*
   paging.cpp
   Boot paging rootines
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
#include "paging.h"
#include "memory.h"
#include "panic.h"

PagingManager64::PagingManager64()
{
	m_pageDirPtrTable = static_cast<uint64_t*>(KernelAllocator::getInstance().allocMemoryPageAlign(PAGE_SIZE));
	auto curPtrTable = reinterpret_cast<const uint64_t*>(cpuGetCR3() & ~PAGE_MASK);
	kmemcpy(m_pageDirPtrTable, curPtrTable, PAGE_SIZE / 2);
	kmemset(&m_pageDirPtrTable[PAGE_SIZE / (sizeof (*m_pageDirPtrTable) * 2)], 0, PAGE_SIZE / 2);

	const uintptr_t memEnd = KernelAllocator::getInstance().getMaxRamAddress();
	for (uintptr_t addr = 0; addr < memEnd; addr += (1 << 30))
		map1GbPage(RAM_VIRTUAL_BASE + addr, addr);
}

void PagingManager64::map1GbPage(uintptr_t virtualBase, uintptr_t physBase)
{
	uint64_t& dirPtrEntryItem = accessToDirPtrEntry(virtualBase);
	if (dirPtrEntryItem != 0)
		panic(L"Multiple allocation 1GB pages");

	if (cpuSupport1GbPages())
	{
		dirPtrEntryItem = (physBase & ~((1LL << 30) - 1)) |
			PAGE_FLAG_PRESENT | PAGE_FLAG_WRITE | PAGE_FLAG_SIZE | PAGE_FLAG_GLOBAL;
	}
	else
	{
		uint64_t* dirPtrEntryItemPtr = static_cast<uint64_t*>(KernelAllocator::getInstance().allocMemoryPageAlign(PAGE_SIZE));
		dirPtrEntryItem = reinterpret_cast<uint64_t>(dirPtrEntryItemPtr) | PAGE_FLAG_PRESENT | PAGE_FLAG_WRITE;
		for (size_t idx = 0; idx < (PAGE_SIZE / sizeof(*dirPtrEntryItemPtr)); ++idx)
		{
			dirPtrEntryItemPtr[idx] = physBase | PAGE_FLAG_PRESENT | PAGE_FLAG_WRITE | PAGE_FLAG_SIZE | PAGE_FLAG_GLOBAL;
			physBase += (2 << 20);
		}
	}
}

uint64_t& PagingManager64::accessToDirPtrEntry(uintptr_t virtualBase)
{
	uint64_t& dirPtrItem = m_pageDirPtrTable[(virtualBase >> 39) & 0x1FF];
	uint64_t* dirPtrItemPtr;
	if (dirPtrItem != 0)
		dirPtrItemPtr = reinterpret_cast<uint64_t*>(dirPtrItem & (CPU_PHYSICAL_ADDRESS_MASK & ~PAGE_MASK));
	else
	{
		dirPtrItemPtr = static_cast<uint64_t*>(KernelAllocator::getInstance().allocMemoryPageAlign(PAGE_SIZE));
		kmemset(dirPtrItemPtr, 0, PAGE_SIZE);
		dirPtrItem = reinterpret_cast<uint64_t>(dirPtrItemPtr) | PAGE_FLAG_PRESENT | PAGE_FLAG_WRITE;
	}

	return dirPtrItemPtr[(virtualBase >> 30) & 0x1FF];
}

uint64_t& PagingManager64::accessToPageTableEntry(uintptr_t virtualBase)
{
	const uint64_t pageDirAreaSize = PAGE_SIZE * PAGE_SIZE / sizeof(uint64_t);
	if ((virtualBase >= (m_currentPageDirBase + pageDirAreaSize)) || (virtualBase < m_currentPageDirBase))
	{
		uint64_t& dirPtrEntryItem = accessToDirPtrEntry(virtualBase);
		uint64_t* dirPtrEntryItemPtr;
		if (dirPtrEntryItem == 0)
		{
			dirPtrEntryItemPtr = static_cast<uint64_t*>(KernelAllocator::getInstance().allocMemoryPageAlign(PAGE_SIZE));
			kmemset(dirPtrEntryItemPtr, 0, PAGE_SIZE);
			dirPtrEntryItem = reinterpret_cast<uint64_t>(dirPtrEntryItemPtr) | PAGE_FLAG_PRESENT | PAGE_FLAG_WRITE;
		}
		else
		{
			if ((dirPtrEntryItem & PAGE_FLAG_GLOBAL) != 0)
				panic(L"Map 4K mage over 1G page");

			dirPtrEntryItemPtr = reinterpret_cast<uint64_t*>(dirPtrEntryItem & (CPU_PHYSICAL_ADDRESS_MASK & ~PAGE_MASK));
		}

		uint64_t& pageDirEntry = dirPtrEntryItemPtr[(virtualBase >> 21) & 0x1FF];
		if (pageDirEntry != 0)
			m_currentPageDir = reinterpret_cast<uint64_t*>(pageDirEntry & (CPU_PHYSICAL_ADDRESS_MASK & ~PAGE_MASK));
		else
		{
			m_currentPageDir = static_cast<uint64_t*>(KernelAllocator::getInstance().allocMemoryPageAlign(PAGE_SIZE));
			kmemset(m_currentPageDir, 0, PAGE_SIZE);
			pageDirEntry = reinterpret_cast<uint64_t>(m_currentPageDir) | PAGE_FLAG_PRESENT | PAGE_FLAG_WRITE;
		}

		m_currentPageDirBase = virtualBase & ~(pageDirAreaSize - 1);
	}
	return m_currentPageDir[(virtualBase >> 12) & 0x1FF];
}

void PagingManager64::mapPage(uintptr_t virtualBase, uintptr_t physBase, bool readOnly, bool wc)
{
	accessToPageTableEntry(virtualBase) = (physBase & ~PAGE_MASK) |
		PAGE_FLAG_PRESENT | (readOnly ? 0 : uintptr_t(PAGE_FLAG_WRITE)) | (wc ? uintptr_t(PAGE_FLAG_WC) : 0) | PAGE_FLAG_GLOBAL;
}

void PagingManager64::protectPages(uintptr_t virtualBase, size_t size)
{
	size = (size + PAGE_MASK) & ~PAGE_MASK;
	uintptr_t pageCnt = size / PAGE_SIZE;
	while (pageCnt-- > 0)
	{
		accessToPageTableEntry(virtualBase) &= ~PAGE_FLAG_WRITE;
		virtualBase += PAGE_SIZE;
	}
}

void* PagingManager64::mapToKernel(const void* physPtr, size_t size, bool readOnly, bool wc)
{
	size = (size + PAGE_MASK) & ~PAGE_MASK;
	uintptr_t physAddr = reinterpret_cast<uintptr_t>(physPtr);
	uintptr_t virtualAddr = m_freeKernelVirtualMemory;
	uintptr_t pageCnt = size / PAGE_SIZE;
	while (pageCnt-- > 0)
	{
		mapPage(virtualAddr, physAddr, readOnly, wc);
		virtualAddr += PAGE_SIZE;
		physAddr += PAGE_SIZE;
	}
	void* ret = reinterpret_cast<void*>(m_freeKernelVirtualMemory);
	m_freeKernelVirtualMemory = virtualAddr;
	return ret;
}

void PagingManager64::storeParams(KernelParams::VirtualMemory& params)
{
	params.m_freeStart = m_freeKernelVirtualMemory;
}

void PagingManager64::setupVirtualMemory()
{
	cpuSetCR3(reinterpret_cast<uintptr_t>(m_pageDirPtrTable));
}

PagingManager64& PagingManager64::getInstance()
{
	static PagingManager64 mgr;
	return mgr;
}

