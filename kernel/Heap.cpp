/*
   Heap.cpp
   Kernel local memory heap
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
#include <kalgorithm.h>
#include <limits>
#include "panic.h"
#include "Heap.h"

enum : uint64_t
{
	HEAP_MARKER_BUSY = 0xFFFFFFFFFFFFFFFFULL,
	HEAP_ALLOC_UNIT_SIZE = 0x101000,
	HEAP_MEM_REGION_LIMIT = 0x100000
};

static const int g_heapMaxIterations = 32;

Heap::Heap(VirtualMemoryManager& vmm)
	: m_vmm(vmm)
{
}

Heap::~Heap()
{
	PANIC(L"not implemented");
}

Heap& Heap::system()
{
	static Heap heap(VirtualMemoryManager::system());
	return heap;
}

void* Heap::allocEqual(MemoryRegionHeader** header, MemoryRegionHeader* region)
{
	if (*header == region)
	{
		*header = region->m_szNext;
	}
	else
	{
		MemoryRegionHeader* regNext = region->m_szNext;
		region->m_szPrev->m_szNext = regNext;
		if (regNext != nullptr)
			regNext->m_szPrev = region->m_szPrev;
	}
	region->m_busyMarker = HEAP_MARKER_BUSY;
	return reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(region) + MemRegionAlign);
}

void* Heap::allocMore(MemoryRegionHeader* region, size_t size)
{
	MemoryRegionHeader* freeRegion = reinterpret_cast<MemoryRegionHeader*>(reinterpret_cast<uintptr_t>(region) + size);
	MemoryRegionHeader* regNext = region->m_addrNext;
	deleteMemRegion(region, cpuLastOneBitIndex(region->size()));
	freeRegion->m_addrPrev = region;
	freeRegion->m_addrNext = regNext;
	region->m_addrNext = freeRegion;
	regNext->m_addrPrev = freeRegion;
	const size_t tblIndex = cpuLastOneBitIndex(freeRegion->size());
	MemoryRegionHeader* hdr = m_freeListTable[tblIndex];
	freeRegion->m_szNext = hdr;
	if (hdr != nullptr)
		hdr->m_szPrev = freeRegion;
	m_freeListTable[tblIndex] = freeRegion;
	region->m_busyMarker = HEAP_MARKER_BUSY;
	return reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(region) + MemRegionAlign);
}

void* Heap::allocNew(size_t size)
{
	MemoryRegionHeader* region = static_cast<MemoryRegionHeader*>(m_vmm.alloc(HEAP_ALLOC_UNIT_SIZE, VMM_READWRITE));
	if (region == nullptr)
		return nullptr;

	m_virtualMemorySize += HEAP_ALLOC_UNIT_SIZE;
	MemoryRegionHeader* freeRegion = reinterpret_cast<MemoryRegionHeader*>(reinterpret_cast<uintptr_t> (region) + size);
	MemoryRegionHeader* nullRegion = reinterpret_cast<MemoryRegionHeader*>(reinterpret_cast<uintptr_t> (region) + HEAP_ALLOC_UNIT_SIZE - MemRegionAlign);
	region->m_addrPrev = nullptr;
	region->m_addrNext = freeRegion;
	freeRegion->m_addrPrev = region;
	freeRegion->m_addrNext = nullRegion;
	nullRegion->m_addrNext = nullptr;
	const size_t regSize = reinterpret_cast<uintptr_t>(nullRegion) - reinterpret_cast<uintptr_t>(freeRegion);
	const size_t tblIndex = cpuLastOneBitIndex(regSize);
	MemoryRegionHeader* hdr = m_freeListTable[tblIndex];
	freeRegion->m_szNext = hdr;
	if (hdr != nullptr)
		hdr->m_szPrev = freeRegion;
	m_freeListTable[tblIndex] = freeRegion;
	region->m_busyMarker = HEAP_MARKER_BUSY;
	return reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(region) + MemRegionAlign);
}

void* Heap::allocHuge(size_t size)
{
	klock_guard lock(m_mutex);
	const size_t virtualSize = cpuAlignAddrHi<size_t>(size + MemRegionAlign);
	MemoryRegionHeader* region = static_cast<MemoryRegionHeader*>(m_vmm.alloc(virtualSize, VMM_READWRITE));
	m_virtualMemorySize += virtualSize;
	MemoryRegionHeader* nullRegion = reinterpret_cast<MemoryRegionHeader*>(reinterpret_cast<uintptr_t>(region) + size);
	region->m_addrPrev = nullptr;
	region->m_addrNext = nullRegion;
	nullRegion->m_addrNext = nullptr;
	region->m_busyMarker = HEAP_MARKER_BUSY;
	return reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(region) + MemRegionAlign);
}

void Heap::deleteMemRegion(MemoryRegionHeader* region, size_t tableIdx)
{
	if (region == m_freeListTable[tableIdx])
	{
		m_freeListTable[tableIdx] = region->m_szNext;
	}
	else
	{
		MemoryRegionHeader* regSzPrev = region->m_szPrev;
		MemoryRegionHeader* regSzNext = region->m_szNext;
		regSzPrev->m_szNext = regSzNext;
		if (regSzNext != nullptr)
			regSzNext->m_szPrev = regSzPrev;
	}
}

void Heap::updateMemRegion(MemoryRegionHeader* region, size_t oldTableIdx)
{
	const size_t tblIndex = cpuLastOneBitIndex(region->size());
	if (tblIndex != oldTableIdx)
	{
		deleteMemRegion(region, oldTableIdx);
		MemoryRegionHeader* hdr = m_freeListTable[tblIndex];
		region->m_szNext = hdr;
		if (hdr != nullptr)
			hdr->m_szPrev = region;
		m_freeListTable[tblIndex] = region;
	}
}

void Heap::addMemRegion(MemoryRegionHeader* region)
{
	const size_t tblIndex = cpuLastOneBitIndex(region->size());
	MemoryRegionHeader* hdr = m_freeListTable[tblIndex];
	region->m_szNext = hdr;
	if (hdr != nullptr)
		hdr->m_szPrev = region;
	m_freeListTable[tblIndex] = region;
}

void Heap::cleanMemRegion(void* ptr, MemoryRegionHeader* region)
{
	const uintptr_t fpBase = cpuAlignPtrHiInt(ptr);
	const uintptr_t fpEnd = cpuAlignPtrLoInt(region->m_addrNext);
	if (fpEnd > fpBase)
		m_vmm.freeRamPages(reinterpret_cast<void*>(fpBase), fpEnd - fpBase);
}

void Heap::cleanMemRegionUN(void* ptr, MemoryRegionHeader* region)
{
	const uintptr_t fpBase = cpuAlignPtrHiInt(ptr);
	const size_t unionAdd = kmin<size_t>(region->m_addrNext->size(), PAGE_MASK);
	const uintptr_t fpEnd = cpuAlignAddrLo<uintptr_t>(reinterpret_cast<uintptr_t>(region->m_addrNext) + unionAdd);
	if (fpEnd > fpBase)
		m_vmm.freeRamPages(reinterpret_cast<void*>(fpBase), fpEnd - fpBase);
}

void Heap::cleanMemRegionUNP(void* ptr, MemoryRegionHeader* region)
{
	uintptr_t fpBase = cpuAlignPtrLoInt(ptr);
	if (fpBase < (reinterpret_cast<uintptr_t> (region->m_addrPrev) + MemRegionAlign))
		fpBase += PAGE_SIZE;
	const size_t unionAdd = kmin<size_t>(region->m_addrNext->size(), PAGE_MASK);
	const uintptr_t fpEnd = cpuAlignAddrLo<uintptr_t>(reinterpret_cast<uintptr_t>(region->m_addrNext) + unionAdd);
	if (fpEnd > fpBase)
		m_vmm.freeRamPages(reinterpret_cast<void*>(fpBase), fpEnd - fpBase);
}

void Heap::cleanMemRegionUP(void* ptr, MemoryRegionHeader* region)
{
	uintptr_t fpBase = cpuAlignPtrLoInt(ptr);
	if (fpBase < (reinterpret_cast<uintptr_t>(region->m_addrPrev) + MemRegionAlign))
		fpBase += PAGE_SIZE;
	const uintptr_t fpEnd = cpuAlignPtrLoInt(region->m_addrNext);
	if (fpEnd > fpBase)
		m_vmm.freeRamPages(reinterpret_cast<void*>(fpBase), fpEnd - fpBase);
}

void* Heap::alloc(size_t size)
{
	if (size == 0)
		return nullptr;

	size = (size + (MemRegionAlign * 2) - 1) & ~(MemRegionAlign - 1);
	if (size >= HEAP_MEM_REGION_LIMIT)
		return allocHuge(size);

	const size_t tblIndex = cpuLastOneBitIndex(size);
	klock_guard lock(m_mutex);
	MemoryRegionHeader* cutListRegion = m_freeListTable[tblIndex];
	if (cutListRegion != nullptr)
	{
		size_t minDelta = std::numeric_limits<size_t>::max();
		MemoryRegionHeader* minRegion = nullptr;
		for (int cnt = 0; (cutListRegion != nullptr) && (cnt < g_heapMaxIterations); cnt++, cutListRegion = cutListRegion->m_szNext)
		{
			const size_t regSize = cutListRegion->size();
			if (regSize >= size)
			{
				const size_t delta = regSize - size;
				if (delta < minDelta)
				{
					if (delta == 0)
						return allocEqual(&m_freeListTable[tblIndex], cutListRegion);

					minRegion = cutListRegion;
					minDelta = delta;
				}
			}
		}
		if (minRegion != nullptr)
			return allocMore(minRegion, size);
	}
	for (size_t idx = tblIndex + 1; idx < HeapLogTableSize; idx++)
	{
		MemoryRegionHeader* region = m_freeListTable[idx];
		if (region != nullptr)
			return allocMore(region, size);
	}

	return allocNew(size);
}

void Heap::free(void* ptr)
{
	klock_guard lock(m_mutex);
	MemoryRegionHeader* region = reinterpret_cast<MemoryRegionHeader*>(reinterpret_cast<uintptr_t>(ptr) - MemRegionAlign);
	MemoryRegionHeader* regNext = region->m_addrNext;
	MemoryRegionHeader* regPrev = region->m_addrPrev;
	MemoryRegionHeader* regNextNext = regNext->m_addrNext;

	if ((regPrev != nullptr) && (regPrev->m_busyMarker != HEAP_MARKER_BUSY))
	{
		const size_t oldTblIndex = cpuLastOneBitIndex(regPrev->size());
		if (regNextNext != nullptr)
		{
			if (regNext->m_busyMarker != HEAP_MARKER_BUSY)
			{
				deleteMemRegion(regNext, cpuLastOneBitIndex(regNext->size()));
				if ((regPrev->m_addrPrev == nullptr) && (regNextNext->m_addrNext == nullptr))
				{
					m_virtualMemorySize -= calcVirtualMemoryBeetween(regPrev, regNextNext);
					deleteMemRegion(regPrev, oldTblIndex);
					m_vmm.free(regPrev);
					return;
				}

				cleanMemRegionUNP(ptr, region);
				regPrev->m_addrNext = regNextNext;
				regNextNext->m_addrPrev = regPrev;
			}
			else
			{
				cleanMemRegionUP(ptr, region);
				regPrev->m_addrNext = regNext;
				regNext->m_addrPrev = regPrev;
			}
		}
		else
		{
			if (regPrev->m_addrPrev == nullptr)
			{
				m_virtualMemorySize -= calcVirtualMemoryBeetween(regPrev, regNext);
				deleteMemRegion(regPrev, oldTblIndex);
				m_vmm.free(regPrev);
				return;
			}

			cleanMemRegionUP(ptr, region);
			regPrev->m_addrNext = regNext;
		}
		updateMemRegion(regPrev, oldTblIndex);
	}
	else if (regNextNext != nullptr)
	{
		if (regNext->m_busyMarker != HEAP_MARKER_BUSY)
		{
			deleteMemRegion(regNext, cpuLastOneBitIndex(regNext->size()));
			if ((regPrev == nullptr) && (regNextNext->m_addrNext == nullptr))
			{
				m_virtualMemorySize -= calcVirtualMemoryBeetween(region, regNextNext);
				m_vmm.free(region);
				return;
			}

			cleanMemRegionUN(ptr, region);
			region->m_addrNext = regNextNext;
			regNextNext->m_addrPrev = region;
			addMemRegion(region);
		}
		else
		{
			cleanMemRegion(ptr, region);
			addMemRegion(region);
		}
	}
	else
	{
		if (regPrev == nullptr)
		{
			m_virtualMemorySize -= calcVirtualMemoryBeetween(region, regNext);
			m_vmm.free(region);
			return;
		}

		cleanMemRegion(ptr, region);
		addMemRegion(region);
	}
}

size_t Heap::calcVirtualMemoryBeetween(MemoryRegionHeader* lower, MemoryRegionHeader* upper)
{
	return (reinterpret_cast<uintptr_t>(upper) - reinterpret_cast<uintptr_t>(lower) + MemRegionAlign);
}
