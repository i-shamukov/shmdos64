/*
   memory.cpp
   Bootloader memory allocation & initialization
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
#include <vmem_utils.h>
#include <conout.h>
#include "memory.h"
#include "panic.h"

static const wchar_t* g_memoryAllocationErrorStr = L"Pages allocation error";

void* KernelAllocator::allocMemoryPageAlign(size_t size)
{
	const auto pages = (size + PAGE_MASK) / PAGE_SIZE;
	EFI_PHYSICAL_ADDRESS addr;
	if (getSystemTable()->BootServices->AllocatePages(AllocateAnyPages, EfiLoaderData, pages, &addr) != EFI_SUCCESS)
	{
		panic(g_memoryAllocationErrorStr);
	}
	m_usedRegions.push_back(PagesRegion{addr, pages});
	return reinterpret_cast<void*>(addr);
}

void KernelAllocator::initMemoryMap()
{
	auto bs = getSystemTable()->BootServices;
	UINTN tableSize = 0;
	UINT32 ver = 0;
	UINTN key = 0;
	UINTN descSize = 0;
	bs->GetMemoryMap(&tableSize, nullptr, &key, &descSize, &ver);
	if (tableSize == 0)
		panic(L"Failed to get size of memory map");
	kvector<char> tableData(tableSize);
	if (bs->GetMemoryMap(&tableSize, reinterpret_cast<EFI_MEMORY_DESCRIPTOR*>(tableData.data()), &key, &descSize, &ver) != EFI_SUCCESS)
		panic(L"Failed to get memory map");

	println(L"Dump memory map:");
	kvector<KernelParams::PhysicalMemory::PhysicalMemoryRegion> tmpRegions;
	for (UINTN offset = 0; offset < tableSize; offset += descSize)
	{
		auto desc = reinterpret_cast<EFI_MEMORY_DESCRIPTOR*>(&tableData[offset]);
		if (desc->Type == 0)
			continue;

		desc->VirtualStart = reinterpret_cast<uintptr_t>(physToVirtualInt<void>(desc->PhysicalStart));
		m_virtualAddressMap.push_back(*desc);
		println(desc->Type, L' ', hex(desc->PhysicalStart, false), ' ', desc->NumberOfPages, ' ', hex(desc->Attribute, false));

		if ((desc->Attribute & 0x0F) != 0xF)
			continue;

		switch (desc->Type)
		{
		case EfiLoaderCode:
		case EfiLoaderData:
		case EfiBootServicesCode:
		case EfiBootServicesData:
		case EfiConventionalMemory:
			break;

		default:
			continue;
		}

		uintptr_t start = desc->PhysicalStart;
		const uintptr_t end = start + desc->NumberOfPages * PAGE_SIZE;
		if (end < CPU_EXTENDED_MEMORY_BASE)
			continue;

		if (start < CPU_EXTENDED_MEMORY_BASE)
			start = CPU_EXTENDED_MEMORY_BASE;

		KernelParams::PhysicalMemory::PhysicalMemoryRegion* pFree = nullptr;
		KernelParams::PhysicalMemory::PhysicalMemoryRegion* pUnite = nullptr;
		for (auto& memReg : tmpRegions)
		{
			if (memReg.m_end == 0)
			{
				pFree = &memReg;
				continue;
			}

			if (memReg.m_end == start)
			{
				memReg.m_end = end;
				pUnite = &memReg;
				break;
			}

			if (memReg.m_start == end)
			{
				memReg.m_start = start;
				pUnite = &memReg;
				break;
			}
		}
		if (pUnite != nullptr)
		{
			for (auto& memReg : tmpRegions)
			{
				if ((memReg.m_end == 0) || (&memReg == pUnite))
					continue;

				if (memReg.m_end == pUnite->m_start)
				{
					memReg.m_end = pUnite->m_end;
					pUnite->m_end = 0;
					break;
				}

				if (memReg.m_start == pUnite->m_end)
				{
					memReg.m_start = pUnite->m_start;
					pUnite->m_end = 0;
					break;
				}
			}
		}
		else if (pFree != nullptr)
		{
			pFree->m_start = start;
			pFree->m_end = end;
		}
		else
		{
			tmpRegions.push_back(KernelParams::PhysicalMemory::PhysicalMemoryRegion{start, end});
		}
	}

	for (const auto& reg : tmpRegions)
		if (reg.m_end != 0)
			m_freeRegions.push_back(reg);
}

uintptr_t KernelAllocator::getMaxRamAddress() const
{
	uintptr_t maxAddr = 0;
	for (const auto& reg : m_freeRegions)
		if (maxAddr < reg.m_end)
			maxAddr = reg.m_end;
	return maxAddr;
}

UINTN KernelAllocator::getMemoryKey()
{
	UINTN tableSize = 0;
	UINT32 ver = 0;
	UINTN key = 0;
	UINTN descSize = 0;
	getSystemTable()->BootServices->GetMemoryMap(&tableSize, nullptr, &key, &descSize, &ver);
	return key;
}

void KernelAllocator::storeBusyRegion(const PagesRegion& useReg, size_t& regCnt)
{
	const uintptr_t start = useReg.m_base;
	const uintptr_t end = start + useReg.m_num * PAGE_SIZE;
	KernelParams::PhysicalMemory::PhysicalMemoryRegion* pFreeReg = nullptr;
	for (auto& freeReg : m_freeRegions)
	{
		if ((start >= freeReg.m_start) && (end <= freeReg.m_end))
		{
			pFreeReg = &freeReg;
			break;
		}
	}
	if (pFreeReg == nullptr)
		panic(L"Match boot memory area witch memory map failed");

	if (start > pFreeReg->m_start)
	{
		if (end < pFreeReg->m_end)
		{
			m_freeRegions.push_back(KernelParams::PhysicalMemory::PhysicalMemoryRegion{end, pFreeReg->m_end});
			regCnt++;
		}
		pFreeReg->m_end = start;
	}
	else
	{
		if (end < pFreeReg->m_end)
			pFreeReg->m_start = end;
		else
		{
			pFreeReg->m_start = 0;
			pFreeReg->m_end = 0;
			regCnt--;
		}
	}
}

void KernelAllocator::storeParams(KernelParams::PhysicalMemory& params)
{
	size_t regCnt = m_freeRegions.size();
	size_t usedRegionIdx = 0;
	for (; usedRegionIdx < m_usedRegions.size(); ++usedRegionIdx)
		storeBusyRegion(m_usedRegions[usedRegionIdx], regCnt);

	const size_t regCntLimit = regCnt + 1;
	allocPageAlign(params.m_physicalMemoryRegions, regCntLimit);

	for (; usedRegionIdx < m_usedRegions.size(); ++usedRegionIdx)
		storeBusyRegion(m_usedRegions[usedRegionIdx], regCnt);

	if (regCnt > regCntLimit)
	{
		panic(L"Recursively increasing memory regions");
	}

	params.m_regions = regCnt;
	auto* pStoreRegs = params.m_physicalMemoryRegions;
	for (auto& freeReg : m_freeRegions)
	{
		if (freeReg.m_start != 0)
		{
			//print(hex(freeReg.m_start, false), ' ', hex(freeReg.m_end, false), '\n');
			pStoreRegs->m_start = freeReg.m_start;
			pStoreRegs->m_end = freeReg.m_end;
			pStoreRegs++;
		}
	}

	params.m_physicalMemoryRegions = physToVirtual(params.m_physicalMemoryRegions);
}

void KernelAllocator::setVirtualAddressMap()
{
	const EFI_STATUS result = getSystemTable()->RuntimeServices->SetVirtualAddressMap(
		m_virtualAddressMap.size() * sizeof(EFI_MEMORY_DESCRIPTOR),
		sizeof(EFI_MEMORY_DESCRIPTOR),
		1,
		m_virtualAddressMap.data());
	if (result != EFI_SUCCESS)
	{
		panic(L"EFI SetVirtualAddressMap failed");
		for (;;);
	}
}

KernelAllocator& KernelAllocator::getInstance()
{
	static KernelAllocator instance;
	return instance;
}