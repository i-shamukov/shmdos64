/*
   VirtualMemoryManager.h
   Shared header for SHM DOS64
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

enum : uintptr_t
{
	VMM_RESERVED = 0x00,
	VMM_NOACCESS = 0x01,
	VMM_READONLY = 0x02,
	VMM_READWRITE = 0x04,
	VMM_EXECUTE = 0x10,
	VMM_NOCACHE = 0x200,
	VMM_COMMIT = 0x10000
};

class PagingManager64;
class VirtualMemoryManagerPrivate;
class VirtualMemoryManager
{
public:
	void* alloc(size_t size, uintptr_t flags);
	bool free(void* pointer);
	void freeRamPages(void* base, size_t size, bool realloc = true);
	void* mapMmio(uintptr_t mmioBase, size_t size, bool cacheDisabled = true);
	bool unmapMmio(void *pointer);
	bool setPagesFlags(void* pointer, size_t size, uintptr_t flags);
	PagingManager64* pagingManager();

	static VirtualMemoryManager& system();

private:
	VirtualMemoryManager(const VirtualMemoryManager&) = delete;
	VirtualMemoryManager(VirtualMemoryManager&&) = delete;
	VirtualMemoryManager();
	~VirtualMemoryManager();

private:
	VirtualMemoryManagerPrivate* m_private;
};
