/*
   Heap.h
   Kernel header
   SHM DOS64
   Copyright (c) 2022, Ilya Shamukov <ilya.shamukov@gmail.com>
   
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

#pragma once
#include <kmutex.h>
#include <VirtualMemoryManager.h>

class Heap
{
public:
	Heap(VirtualMemoryManager& vmm);
	~Heap();
	void* alloc(size_t size);
	void free(void* ptr);
	size_t virtualMemorySize() const
	{
		return m_virtualMemorySize;
	}
	static Heap& system();

private:
	struct MemoryRegionHeader
	{
		union
		{
			MemoryRegionHeader* m_szNext;
			uintptr_t m_busyMarker;
		};
		MemoryRegionHeader* m_szPrev;
		MemoryRegionHeader* m_addrNext;
		MemoryRegionHeader* m_addrPrev;

		size_t size() const
		{
			return ((uintptr_t) m_addrNext - (uintptr_t)this);
		}
	};
	static_assert(sizeof (MemoryRegionHeader) == 4 * sizeof (uintptr_t));
	static const size_t HeapLogTableSize = 21;
	static const size_t MemRegionAlign = sizeof (MemoryRegionHeader);

private:
	void* allocEqual(MemoryRegionHeader** header, MemoryRegionHeader* region);
	void* allocMore(MemoryRegionHeader* region, size_t size);
	void* allocNew(size_t size);
	void* allocHuge(size_t size);
	void deleteMemRegion(MemoryRegionHeader* region, size_t tableIdx);
	void updateMemRegion(MemoryRegionHeader* region, size_t oldTableIdx);
	void addMemRegion(MemoryRegionHeader* region);
	void cleanMemRegion(void* ptr, MemoryRegionHeader* region);
	void cleanMemRegionUN(void* ptr, MemoryRegionHeader* region);
	void cleanMemRegionUNP(void* ptr, MemoryRegionHeader* region);
	void cleanMemRegionUP(void* ptr, MemoryRegionHeader* region);
	static size_t calcVirtualMemoryBeetween(MemoryRegionHeader* lower, MemoryRegionHeader* upper);

private:
	Heap(const Heap& orig) = delete;
	Heap(Heap&&) = delete;
	VirtualMemoryManager& m_vmm;
	MemoryRegionHeader* m_freeListTable[HeapLogTableSize] = {};
	size_t m_virtualMemorySize = 0;
	kmutex m_mutex;
};
