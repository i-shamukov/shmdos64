/*
   memory.h
   Header for EFI BootLoader
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
#include <common_types.h>
#include <kernel_params.h>
#include <kvector.h>
#include "efi.h"

class KernelAllocator
{
public:
	static KernelAllocator& getInstance();
	void* allocMemoryPageAlign(size_t size);
	void initMemoryMap();
	UINTN getMemoryKey();
	uintptr_t getMaxRamAddress() const;
	void storeParams(KernelParams::PhysicalMemory& params);
	void setVirtualAddressMap();
	template<typename T> void allocPageAlign(T*& ptr, size_t size)
	{
		ptr = static_cast<T*> (allocMemoryPageAlign(size * sizeof (*ptr)));
	}

private:
	struct PagesRegion
	{
		uintptr_t m_base;
		size_t m_num;
	};

private:
	KernelAllocator()
	{
	}
	KernelAllocator(const KernelAllocator&) = delete;
	KernelAllocator(KernelAllocator&&) = delete;
	void storeBusyRegion(const PagesRegion& region, size_t& regCnt);

private:
	kvector<PagesRegion> m_usedRegions;
	kvector<EFI_MEMORY_DESCRIPTOR> m_virtualAddressMap;
	kvector<KernelParams::PhysicalMemory::PhysicalMemoryRegion> m_freeRegions;
};
