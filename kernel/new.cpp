/*
   new.cpp
   Kernel C++ new allocator & page heap debugger
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

#include <kernel_export.h>
#include "Heap.h"
#include "paging.h"

//#define PAGE_HEAP

#ifdef PAGE_HEAP
#include <cpu.h>
#include <VirtualMemoryManager.h>

void* allocPageHeap(size_t size)
{
	uintptr_t addr = reinterpret_cast<uintptr_t> (VirtualMemoryManager::system().alloc(size + PAGE_SIZE, VMM_READWRITE));
	addr += (PAGE_SIZE - size) & PAGE_MASK;
	PagingManager64::system().mapPage(addr + size, 0, 0);
	return reinterpret_cast<void*> (addr);
}

void freePageHeap(void *ptr)
{
	VirtualMemoryManager::system().free(reinterpret_cast<void*> (reinterpret_cast<uintptr_t> (ptr) & ~PAGE_MASK));
}
#endif

KERNEL_SHARED void* operator new[](size_t size) 
{
#ifndef PAGE_HEAP
	return Heap::system().alloc(size);
#else
	return allocPageHeap(size);
#endif
}

KERNEL_SHARED void operator delete[](void* ptr) 
{
#ifndef PAGE_HEAP
	Heap::system().free(ptr);
#else
	freePageHeap(ptr);
#endif
}

KERNEL_SHARED void* operator new(size_t size)
{
#ifndef PAGE_HEAP
	return Heap::system().alloc(size);
#else
	return allocPageHeap(size);
#endif
}

KERNEL_SHARED void operator delete(void* ptr)
{
#ifndef PAGE_HEAP
	Heap::system().free(ptr);
#else
	freePageHeap(ptr);
#endif
}

KERNEL_SHARED void operator delete(void* ptr, size_t)
{
#ifndef PAGE_HEAP
	Heap::system().free(ptr);
#else
	freePageHeap(ptr);
#endif
}