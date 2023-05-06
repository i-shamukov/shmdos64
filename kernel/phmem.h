/*
   phmem.h
   Kernel header
   SHM DOS64
   Copyright (c) 2023, Ilya Shamukov <ilya.shamukov@gmail.com>
   
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
#include <common_types.h>
#include <vmem_utils.h>

class RamAllocator
{
public:
	void* allocPagePtr(bool memzero);
	void freePagePtr(void* addr);
	static RamAllocator& getInstance();

	uintptr_t allocPage(bool memzero)
	{
		return virtualToPhysInt(allocPagePtr(memzero));
	}

	void freePage(uintptr_t addr)
	{
		return freePagePtr(physToVirtualInt<void>(addr));
	}

	template<typename T>
	T* allocPagePtrCast(bool memzero)
	{
		return static_cast<T*> (allocPagePtr(memzero));
	}

	size_t avaibleRamSize() const
	{
		return m_avaibleRamSize;
	}

	uintptr_t getMaxRamAddr() const 
	{
		return m_maxRamAddr;
	}

private:
	RamAllocator();
	RamAllocator(const RamAllocator&) = delete;
	RamAllocator(RamAllocator&&) = delete;

private:
	struct alignas(16) RegionListHeader
	{
		void** m_head;
		uintptr_t m_refCnt;
	}
	volatile m_header{ m_pEndList, 0};
	static_assert((sizeof (m_header) == 2 * sizeof (uintptr_t)), "size of RegionListHeader is incorrect");
	size_t m_avaibleRamSize = 0;
	uintptr_t m_maxRamAddr = 0;

private:
	static void** m_pEndList;
};