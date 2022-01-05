/*
   phmem.cpp
   Lock-free RAM pages allocator
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

#include <cpu.h>
#include <conout.h>
#include "panic.h"
#include "phmem.h"

static void* g_endList = nullptr;
void** RamAllocator::m_pEndList = &g_endList;

RamAllocator::RamAllocator()
{
	print(L"Initializing RAM pages... ");
	const auto& params = getKernelParams()->m_physicalMemory;
	for (size_t idx = 0; idx < params.m_regions; idx++)
	{
		auto& region = params.m_physicalMemoryRegions[idx];
		void** startPtr = physToVirtualInt<void*>(region.m_start);
		void** ptr = startPtr;
		const size_t regSize = region.m_end - region.m_start;
		m_avaibleRamSize += regSize;
		size_t pageCnt = regSize / PAGE_SIZE;
		if (pageCnt == 0)
			continue;

		while (--pageCnt > 0)
		{
			void** nextPtr = ptr + (PAGE_SIZE / sizeof (ptr));
			*ptr = nextPtr;
			ptr = nextPtr;
		}
		*ptr = m_header.m_head;
		m_header.m_head = startPtr;
	}
	println(L"OK (", (m_avaibleRamSize >> 20), L"MB)");
}

void* RamAllocator::allocPagePtr(bool memzero)
{
	RegionListHeader header;
	void** ret;
	do
	{
		header.m_head = m_header.m_head;
		header.m_refCnt = m_header.m_refCnt;
		if (header.m_head == m_pEndList)
			PANIC(L"No enough memory");

		ret = header.m_head;
	}
	while (!cpuInterlockedCompareExchange128(&m_header, header.m_refCnt + 1, *header.m_head, &header));
	if (memzero)
		kmemset(ret, 0, PAGE_SIZE);
	return static_cast<void*>(ret);
}

void RamAllocator::freePagePtr(void* addr)
{
	RegionListHeader header;
	do
	{
		header.m_head = m_header.m_head;
		header.m_refCnt = m_header.m_refCnt;
		*static_cast<void**>(addr) = header.m_head;
	}
	while (!cpuInterlockedCompareExchange128(&m_header, header.m_refCnt + 1, addr, &header));
}

RamAllocator& RamAllocator::getInstance()
{
	static RamAllocator instance;
	return instance;
}