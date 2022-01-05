/*
   VirtualMemoryManager.cpp
   Kernel virtual memory allocator & paging routines
   SHM DOS64
   Copyright (c) 2022, Ilya Shamukov, ilya.shamukov@gmail.com
   
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

#include <limits>
#include <cpu.h>
#include <kernel_params.h>
#include <kalgorithm.h>
#include <kclib.h>
#include "phmem.h"
#include "VirtualMemoryManager_p.h"
#include "panic.h"

#include <conout.h>

enum : uintptr_t
{
	VMM_REG_SIZE_SHIFT = 12,
	VMM_REG_PTR_MASK = (~3ULL),
	VMM_REG_ALLOC_FLAG = 1,
	VMM_REG_BUSY_FLAG = 2,
	VMM_REG_ALL_FLAGS = (VMM_REG_ALLOC_FLAG | VMM_REG_BUSY_FLAG)
};

static const size_t g_maxAllocIterations = 32;
static const uintptr_t g_invalidPageOffset = std::numeric_limits<uintptr_t>::max();

VirtualMemoryManager::VirtualMemoryManager()
{
	static VirtualMemoryManagerPrivate vmmp;
	m_private = &vmmp;
}

VirtualMemoryManager::~VirtualMemoryManager()
{
	PANIC(L"not implemented");
}

VirtualMemoryManager& VirtualMemoryManager::system()
{
	static VirtualMemoryManager vmm;
	return vmm;
}

void* VirtualMemoryManager::alloc(size_t size, uintptr_t flags)
{
	return m_private->alloc(size, flags);
}

bool VirtualMemoryManager::free(void* pointer)
{
	return m_private->free(pointer);
}

void VirtualMemoryManager::freeRamPages(void* base, size_t size, bool realloc)
{
	return m_private->freeRamPages(base, size, realloc);
}

void* VirtualMemoryManager::mapMmio(uintptr_t mmioBase, size_t size, bool cacheDisabled)
{
	return m_private->mapMmio(mmioBase, size, cacheDisabled);
}

bool VirtualMemoryManager::unmapMmio(void* pointer)
{
	return m_private->unmapMmio(pointer);
}

bool VirtualMemoryManager::setPagesFlags(void* base, size_t size, uintptr_t flags)
{
	return m_private->setPagesFlags(base, size, flags);
}

PagingManager64* VirtualMemoryManager::pagingManager()
{
	return m_private->pagingManager();
}

VirtualMemoryManagerPrivate::VirtualMemoryManagerPrivate()
	: m_paging(PagingManager64::system())
	, m_pageDefaultFlag(PAGE_FLAG_GLOBAL)
{
	uintptr_t freeStart = getKernelParams()->m_virtualMemory.m_freeStart;
	const size_t needAllMemory = kmin(RamAllocator::getInstance().avaibleRamSize() + KERNEL_MIN_VIRTUAL_MEMORY, static_cast<size_t>(KERNEL_MAX_VIRTUAL_MEMORY));
	const size_t alreadyUsedInit = freeStart - KERNEL_VIRTUAL_BASE;
	if (alreadyUsedInit > needAllMemory)
		PANIC(L"Incorrect kernel virtual memory settings");

	const size_t needPages = (needAllMemory - alreadyUsedInit) / PAGE_SIZE;
	const size_t regionsSize = cpuAlignAddrHi<size_t>((needPages + 1) * sizeof(uintptr_t));
	m_paging.setPagesFlags(freeStart, regionsSize, PAGE_MASK, PAGE_FLAG_ALLOCATED | PAGE_FLAG_WRITE | PAGE_FLAG_MEMZERO);
	m_memoryRegions = reinterpret_cast<uintptr_t*>(freeStart);
	freeStart += regionsSize;
	const size_t freeListSize = cpuAlignAddrHi<size_t>(needPages * sizeof(VmmFreeMemoryListItem));
	m_paging.setPagesFlags(freeStart, freeListSize, PAGE_MASK, PAGE_FLAG_ALLOCATED | PAGE_FLAG_WRITE | PAGE_FLAG_MEMZERO);
	m_freeListItemsStart = reinterpret_cast<VmmFreeMemoryListItem*>(freeStart);
	m_freeListTableSize = cpuLastOneBitIndex(needPages) + 1;
	kmemset(m_freeListTable, 0, sizeof(m_freeListTable));
	kmemset(m_freeListTableCounters, 0, sizeof(m_freeListTableCounters));
	freeStart += freeListSize;
	m_virtualPages = needPages - ((regionsSize + freeListSize) / PAGE_SIZE);
	m_virtualBase = freeStart;
	addFreeRegion(0, m_virtualPages);
}

void VirtualMemoryManagerPrivate::addFreeRegion(uintptr_t pageBase, size_t numPages)
{
	VmmFreeMemoryListItem* item;
	if (m_freeListItems == nullptr)
		item = m_freeListItemsStart++;
	else
	{
		item = m_freeListItems;
		m_freeListItems = item->m_next;
	}
	const size_t tblIndex = cpuLastOneBitIndex(numPages);
	item->m_numPages = numPages;
	item->m_pageBase = pageBase;
	item->m_prev = nullptr;
	VmmFreeMemoryListItem* flt = m_freeListTable[tblIndex];
	item->m_next = flt;
	if (flt != nullptr)
		flt->m_prev = item;
	m_freeListTable[tblIndex] = item;
	m_freeListTableCounters[tblIndex]++;
	m_memoryRegions[pageBase] = reinterpret_cast<uintptr_t>(item);
	if (numPages > 1)
		m_memoryRegions[pageBase + numPages - 1] = reinterpret_cast<uintptr_t>(item);
}

void VirtualMemoryManagerPrivate::deleteFreeRegion(size_t tableIndex, VmmFreeMemoryListItem* region)
{
	if (region == m_freeListTable[tableIndex])
		m_freeListTable[tableIndex] = region->m_next;
	else
	{
		VmmFreeMemoryListItem* rNext = region->m_next;
		region->m_prev->m_next = rNext;
		if (rNext != nullptr)
			rNext->m_prev = region->m_prev;
	}
	region->m_next = m_freeListItems;
	m_freeListItems = region;
	m_freeListTableCounters[tableIndex]--;
}

void VirtualMemoryManagerPrivate::setBusyRegion(unsigned tableIndex, VmmFreeMemoryListItem* region, size_t numPages)
{
	const uintptr_t pagesBase = region->m_pageBase;
	deleteFreeRegion(tableIndex, region);
	m_memoryRegions[pagesBase] = (numPages << VMM_REG_SIZE_SHIFT) | VMM_REG_BUSY_FLAG;
	if (numPages > 1)
		m_memoryRegions[pagesBase + numPages - 1] = VMM_REG_BUSY_FLAG;
}

uintptr_t VirtualMemoryManagerPrivate::allocPages(size_t numPages)
{
	const size_t tblIndex = cpuLastOneBitIndex(numPages);
	const size_t maxIterateNodes = kmin(m_freeListTableCounters[tblIndex], g_maxAllocIterations);
	VmmFreeMemoryListItem* curListItem = nullptr;
	if (maxIterateNodes > 0)
	{
		VmmFreeMemoryListItem* minItem = nullptr;
		curListItem = m_freeListTable[tblIndex];
		size_t minDelta = std::numeric_limits<size_t>::max();
		for (size_t idx = 0; idx < maxIterateNodes; idx++)
		{
			if (curListItem->m_numPages >= numPages)
			{
				const uintptr_t delta = curListItem->m_numPages - numPages;
				if (delta == 0)
				{
					const uintptr_t retPb = curListItem->m_pageBase;
					setBusyRegion(tblIndex, curListItem, numPages);
					return retPb;
				}
				else if (delta < minDelta)
				{
					minItem = curListItem;
					minDelta = delta;
				}
			}
			curListItem = curListItem->m_next;
		}
		if (minItem != nullptr)
		{
			const uintptr_t retPb = minItem->m_pageBase;
			setBusyRegion(tblIndex, minItem, numPages);
			addFreeRegion(minItem->m_pageBase + numPages, minDelta);
			return retPb;
		}
	}
	for (size_t idx = tblIndex + 1; idx < m_freeListTableSize; idx++)
	{
		if (m_freeListTableCounters[idx] > 0)
		{
			VmmFreeMemoryListItem* item = m_freeListTable[idx];
			const uintptr_t retPb = item->m_pageBase;
			setBusyRegion(idx, item, numPages);
			addFreeRegion(item->m_pageBase + numPages, item->m_numPages - numPages);
			return retPb;
		}
	}
	while (curListItem != nullptr)
	{
		if (curListItem->m_numPages >= numPages)
		{
			const uintptr_t retPb = curListItem->m_pageBase;
			setBusyRegion(tblIndex, curListItem, numPages);
			return retPb;
		}
		curListItem = curListItem->m_next;
	}

	return g_invalidPageOffset;
}

uintptr_t VirtualMemoryManagerPrivate::freePages(uintptr_t pageBase)
{
	size_t numPages = m_memoryRegions[pageBase];
	if ((numPages & VMM_REG_BUSY_FLAG) == 0)
		return 0;

	numPages >>= VMM_REG_SIZE_SHIFT;
	if (numPages == 0)
		return 0;

	uintptr_t regBase = pageBase;
	uintptr_t regSize = numPages;
	if (pageBase > 0)
	{
		uintptr_t listPtr = m_memoryRegions[pageBase - 1];
		if ((listPtr & VMM_REG_BUSY_FLAG) == 0)
		{
			listPtr &= VMM_REG_PTR_MASK;
			if (listPtr != 0)
			{
				VmmFreeMemoryListItem* item = reinterpret_cast<VmmFreeMemoryListItem*>(listPtr);
				const size_t itemNumPages = item->m_numPages;
				regBase = item->m_pageBase;
				regSize += itemNumPages;
				const size_t tableIdx = cpuLastOneBitIndex(itemNumPages);
				deleteFreeRegion(tableIdx, item);
				m_memoryRegions[pageBase] = 0;
				if (itemNumPages > 1)
					m_memoryRegions[pageBase - 1] = 0;
			}
		}
	}
	uintptr_t listPtr = m_memoryRegions[pageBase + numPages];
	if ((listPtr & VMM_REG_BUSY_FLAG) == 0)
	{
		listPtr &= VMM_REG_PTR_MASK;
		if (listPtr != 0)
		{
			VmmFreeMemoryListItem* item = reinterpret_cast<VmmFreeMemoryListItem*>(listPtr);
			const size_t itemNumPages = item->m_numPages;
			regSize += itemNumPages;
			deleteFreeRegion(cpuLastOneBitIndex(itemNumPages), item);
			m_memoryRegions[item->m_pageBase - 1] = 0;
			m_memoryRegions[item->m_pageBase] = 0;
		}
	}
	addFreeRegion(regBase, regSize);
	return numPages;
}

void* VirtualMemoryManagerPrivate::alloc(size_t size, uintptr_t flags)
{
	uintptr_t pflags = m_pageDefaultFlag;
	size_t numPages = (size + PAGE_MASK) / PAGE_SIZE;
	if (numPages == 0)
		return nullptr;

	klock_guard lock(m_mutex);
	const uintptr_t base = allocPages(numPages);
	if (base == g_invalidPageOffset)
		return nullptr;

	const uintptr_t vBase = m_virtualBase + (base * PAGE_SIZE);
	void* pointer = reinterpret_cast<void*>(vBase);
	if (flags & VMM_NOCACHE)
		pflags |= PAGE_FLAG_CACHE_DISABLE;
	if (flags & VMM_COMMIT)
	{
		if (flags & VMM_READWRITE)
			pflags |= PAGE_FLAG_WRITE | PAGE_FLAG_PRESENT;
		else
			pflags |= PAGE_FLAG_PRESENT;

		uintptr_t curPageBase = vBase;
		RamAllocator& allocator = RamAllocator::getInstance();
		while (numPages-- > 0)
		{
			const uintptr_t physBase = allocator.allocPage(false);
			if (physBase == g_invalidPageOffset)
			{
				m_paging.freeRamPages(vBase, curPageBase - vBase, false);
				freePages(base);
				return nullptr;
			}

			m_paging.mapPage(curPageBase, physBase, pflags);
			curPageBase += PAGE_SIZE;
		}
		m_memoryRegions[base] |= VMM_REG_ALLOC_FLAG;
	}
	else
	{
		if (flags & VMM_READWRITE)
			pflags |= PAGE_FLAG_ALLOCATED | PAGE_FLAG_WRITE;
		else if (flags & VMM_READONLY)
			pflags |= PAGE_FLAG_ALLOCATED;
		else
			return pointer;
		m_paging.setPagesFlags(vBase, size, PAGE_MASK, pflags);
		m_memoryRegions[base] |= VMM_REG_ALLOC_FLAG;
	}
	return pointer;
}

bool VirtualMemoryManagerPrivate::free(void* pointer)
{
	uintptr_t base = reinterpret_cast<uintptr_t>(pointer);
	if (base < m_virtualBase)
		return false;

	base = (base - m_virtualBase) / PAGE_SIZE;
	if (base >= m_virtualPages)
		return false;

	klock_guard lock(m_mutex);
	const bool free = m_memoryRegions[base] & VMM_REG_ALLOC_FLAG;
	const size_t numPages = freePages(base);
	if (numPages == 0)
		return false;

	if (free)
		m_paging.freeRamPages(reinterpret_cast<uintptr_t>(pointer), numPages * PAGE_SIZE, false);
	return true;
}

void VirtualMemoryManagerPrivate::freeRamPages(void* base, size_t size, bool realloc)
{
	if (size <= PAGE_SIZE)
		m_paging.freeRamPage(reinterpret_cast<uintptr_t>(base), realloc);
	else
		m_paging.freeRamPages(reinterpret_cast<uintptr_t>(base), size, realloc);
}

void* VirtualMemoryManagerPrivate::mapMmio(uintptr_t mmioBase, size_t size, bool cacheDisabled)
{
	const size_t alignSize = (mmioBase & PAGE_MASK) + size;
	const uintptr_t vBase = reinterpret_cast<uintptr_t>(alloc(alignSize, VMM_RESERVED));
	if (vBase == 0)
		return nullptr;

	const uintptr_t pageFlags = PAGE_FLAG_PRESENT | PAGE_FLAG_WRITE | m_pageDefaultFlag | (cacheDisabled ? PAGE_FLAG_CACHE_DISABLE : 0);
	if (alignSize <= PAGE_SIZE)
		m_paging.mapPage(vBase, mmioBase, pageFlags);
	else
		m_paging.mapPages(vBase, mmioBase, alignSize, pageFlags);
	return reinterpret_cast<void*> (vBase + (mmioBase & PAGE_MASK));
}

bool VirtualMemoryManagerPrivate::unmapMmio(void* pointer)
{
	uintptr_t base = reinterpret_cast<uintptr_t>(pointer);
	if (base < m_virtualBase)
		return false;

	base = (base - m_virtualBase) / PAGE_SIZE;
	if (base >= m_virtualPages)
		return false;

	klock_guard lock(m_mutex);
	const size_t numPages = freePages(base);
	if (numPages == 0)
		return false;

	if (numPages == 1)
		m_paging.mapPage(reinterpret_cast<uintptr_t>(pointer), 0, 0);
	else
		m_paging.setPagesFlags(reinterpret_cast<uintptr_t>(pointer), numPages, static_cast<uintptr_t>(-1), 0);
	return true;
}

bool VirtualMemoryManagerPrivate::setPagesFlags(void* pointer, size_t size, uintptr_t flags)
{
	uintptr_t base = reinterpret_cast<uintptr_t>(pointer);
	if (base < m_virtualBase)
		return false;
	
	base -= m_virtualBase;
	const uintptr_t end = (base + size + PAGE_MASK) / PAGE_SIZE;
	base = base / PAGE_SIZE;
	if (end > m_virtualPages)
		return false;
	
	const uintptr_t numPages = end - base;
	uintptr_t regNumPages = m_memoryRegions[base];
	if( (regNumPages & VMM_REG_BUSY_FLAG) == 0)
		return false;
	
	regNumPages >>= VMM_REG_SIZE_SHIFT;
	if(numPages > regNumPages)
		return false;

	uintptr_t pflags = 0x00;
	if( (flags & VMM_NOACCESS) != 0)
		pflags = 0x00;
	else if( (flags & VMM_READONLY) != 0)
		pflags = PAGE_FLAG_PRESENT;
	else 
		flags = PAGE_FLAG_PRESENT | PAGE_FLAG_WRITE;
	if( (flags & VMM_NOCACHE) != 0)
		pflags |= PAGE_FLAG_CACHE_DISABLE;
	const uintptr_t pageMask = PAGE_FLAG_PRESENT | PAGE_FLAG_WRITE | PAGE_FLAG_CACHE_DISABLE;
	m_paging.setPagesFlags(reinterpret_cast<uintptr_t>(base), numPages * PAGE_SIZE, pageMask, pflags);
	return true;
}

PagingManager64* VirtualMemoryManagerPrivate::pagingManager()
{
	return &m_paging;
}

