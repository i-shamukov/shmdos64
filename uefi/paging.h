/*
   paging.h
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

class PagingManager64
{
public:
	static PagingManager64& getInstance();
	void* mapToKernel(const void* physPtr, size_t size, bool readOnly);
	void storeParams(KernelParams::VirtualMemory& params);
	void protectPages(uintptr_t virtualBase, size_t size);
	void setupVirtualMemory();

private:
	PagingManager64();
	PagingManager64(const PagingManager64&) = delete;
	PagingManager64(PagingManager64&&) = delete;
	void map1GbPage(uintptr_t virtualBase, uintptr_t physBase);
	void mapPage(uintptr_t virtualBase, uintptr_t physBase, bool readOnly);
	uint64_t& accessToDirPtrEntry(uintptr_t virtualBase);
	uint64_t& accessToPageTableEntry(uintptr_t virtualBase);

private:
	uint64_t* m_pageDirPtrTable;

	uint64_t* m_currentPageDir = nullptr;
	uintptr_t m_currentPageDirBase = 0;
	uintptr_t m_freeKernelVirtualMemory = KERNEL_VIRTUAL_BASE;
};
