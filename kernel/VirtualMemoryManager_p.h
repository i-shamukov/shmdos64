/*
   VirtualMemoryManager_p.h
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
#include <kmutex.h>
#include <VirtualMemoryManager.h>
#include "paging.h"

struct VmmFreeMemoryListItem
{
	uintptr_t m_numPages;
	uintptr_t m_pageBase;
	VmmFreeMemoryListItem* m_prev;
	VmmFreeMemoryListItem* m_next;
};

class VirtualMemoryManagerPrivate
{
public:
	VirtualMemoryManagerPrivate();
	void* alloc(size_t size, uintptr_t flags);
	bool free(void* pointer);
	void freeRamPages(void* base, size_t size, bool realloc);
	void* mapMmio(uintptr_t mmioBase, size_t size, MemoryType memoryType);
	bool unmapMmio(void* pointer);
	bool setPagesFlags(void* pointer, size_t size, uintptr_t flags);
	PagingManager64* pagingManager();

private:
	VirtualMemoryManagerPrivate(const VirtualMemoryManager&) = delete;
	VirtualMemoryManagerPrivate(VirtualMemoryManager&&) = delete;
	void addFreeRegion(uintptr_t pageBase, size_t numPages);
	void deleteFreeRegion(size_t tableIndex, VmmFreeMemoryListItem* region);
	void setBusyRegion(unsigned tableIndex, VmmFreeMemoryListItem* region, size_t numPages);
	uintptr_t allocPages(size_t numPages);
	uintptr_t freePages(uintptr_t pageBase);

private:
	PagingManager64& m_paging;
	uintptr_t* m_memoryRegions = nullptr;
	VmmFreeMemoryListItem* m_freeListTable[sizeof(uintptr_t) * 8];
	size_t m_freeListTableCounters[sizeof (uintptr_t) * 8];
	size_t m_freeListTableSize = 0;
	VmmFreeMemoryListItem* m_freeListItems = nullptr;
	VmmFreeMemoryListItem* m_freeListItemsStart = nullptr;
	uintptr_t m_virtualBase = 0;
	uintptr_t m_virtualPages = 0;
	const uintptr_t m_pageDefaultFlag = 0;
	kmutex m_mutex;
};
