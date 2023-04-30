/*
   paging.h
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
#include "SpinLock.h"

class PagingManager64
{
public:
	typedef void (*PageProc)(uint64_t&);

public:
	static PagingManager64& system();
	PagingManager64(bool system);

	void mapPages(uintptr_t virtualBase, uintptr_t physBase, size_t size, uint64_t pageFlags);
	void setPagesFlags(uintptr_t virtualBase, size_t size, uint64_t mask, uint64_t pageFlags);
	void freeRamPages(uintptr_t virtualBase, size_t size, bool virtualAllocated);
	void freeRamPage(uintptr_t virtualBase, bool virtualAllocated);
	void processPages(uintptr_t virtualBase, size_t size, PageProc callback, uint64_t dirFlags = 0);
	void mapPage(uintptr_t virtualBase, uintptr_t physBase, uint64_t pageFlags);
	void processPage(uintptr_t virtualBase, PageProc callback, uint64_t dirFlags = 0);
	void storeRootTable(void* table);

	static void unmapBootPages();
	static PagingManager64* current();
	static void setCurrent(PagingManager64* pagingMgr);
	static void initSmp();
	void initCpu(unsigned int cpuId);

	bool onPageFault(uintptr_t addr, uint64_t errorCode);

private:
	PagingManager64(const PagingManager64&) = delete;
	PagingManager64(PagingManager64&&) = delete;
	~PagingManager64();

	template<typename Runnable>
	void processPagesLevel(unsigned int level, uint64_t* dir, uint64_t pageStart, uint64_t pageEnd, uint64_t dirFlags, Runnable callback, bool& needShootdown);
	template<typename Runnable>
	void processPages(uintptr_t virtualBase, size_t size, uint64_t dirFlags, Runnable callback);
	template<typename Runnable>
	void processPage(uint64_t virtualBase, uint64_t dirFlags, Runnable callback);
	void flushPagesTlb(uintptr_t virtualBase, size_t size, bool needShootdown);

private:
	enum
	{
		Levels = 4
	};
	uint64_t* m_dir256tb = nullptr;
	uint64_t m_cr3;
	QueuedSpinLock m_spin;
	const bool m_system;
};