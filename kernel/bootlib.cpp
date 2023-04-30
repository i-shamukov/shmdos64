/*
   bootlib.cpp
   Memory allocation in boot stage
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

#include <kernel_params.h>
#include <cpu.h>
#include "bootlib.h"
#include "paging.h"
#include "phmem.h"

namespace BootLib
{
	void* virtualAlloc(size_t size, bool memzero)
	{
		const uintptr_t vaddr = getKernelParams()->m_virtualMemory.m_freeStart;
		const uintptr_t endAddr = vaddr + size;
		uintptr_t pageAddr;
		for (pageAddr = vaddr; pageAddr < endAddr; pageAddr += PAGE_SIZE)
			PagingManager64::system().mapPage(pageAddr, RamAllocator::getInstance().allocPage(memzero), PAGE_FLAG_PRESENT | PAGE_FLAG_WRITE | PAGE_FLAG_GLOBAL);
		getKernelParams()->m_virtualMemory.m_freeStart = pageAddr;
		return reinterpret_cast<void*>(vaddr);
	}
}