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
