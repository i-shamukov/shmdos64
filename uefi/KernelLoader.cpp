/*
   KernelLoader.cpp
   Kernel loader
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

#include <cpu.h>
#include "KernelLoader.h"
#include "BootIo.h"
#include "panic.h"
#include "memory.h"
#include "paging.h"

static const wchar_t* g_incorrectKernelFormatStr = L"Kernel file format is incorrect";
static const wchar_t* g_incorrectKernelImageArchitecture = L"Kernel file CPU architecture is not supported";
static const wchar_t* g_unknownRelocStr = L"Unknown kernel relocation type";
static size_t maxKernelImageSize = 0x10000000;

KernelLoader::KernelLoader(const kwstring& kernelName, KernelParams* params)
{
	kvector<char> kernelData;
	if (!BootIo::getInstance().readFile(kernelName.c_str(), 0x1000000, &kernelData))
		panic(L"Failed to read kernel image");

	loadFromMemory(kernelData);

	auto stackPhys = KernelAllocator::getInstance().allocMemoryPageAlign(KERNEL_STACK_SIZE);
	auto stackVirtual = PagingManager64::getInstance().mapToKernel(stackPhys, KERNEL_STACK_SIZE, false);
	m_stack = reinterpret_cast<uintptr_t>(stackVirtual);
	params->m_stackBase = m_stack;
}

void KernelLoader::loadFromMemory(kvector<char>& moduleData)
{
	auto pbModule = reinterpret_cast<uint8_t*>(moduleData.data());

	auto dosHdr = reinterpret_cast<PIMAGE_DOS_HEADER>(moduleData.data());
	if ((moduleData.size() <= sizeof(*dosHdr)) ||
		(dosHdr->e_magic != IMAGE_DOS_SIGNATURE) ||
		(dosHdr->e_lfanew == 0) ||
		(dosHdr->e_lfanew > (moduleData.size() - sizeof(IMAGE_NT_HEADERS) + sizeof(IMAGE_OPTIONAL_HEADER)))
		)
	{
		panic(g_incorrectKernelFormatStr);
	}

	auto peHdr = reinterpret_cast<PIMAGE_NT_HEADERS>(&pbModule[dosHdr->e_lfanew]);
	m_imageSize = peHdr->OptionalHeader.SizeOfImage;
	if ((peHdr->Signature != IMAGE_NT_SIGNATURE) ||
		(m_imageSize > maxKernelImageSize) ||
		(peHdr->OptionalHeader.SectionAlignment < PAGE_SIZE)
		)
	{
		panic(g_incorrectKernelFormatStr);
	}

	if (peHdr->FileHeader.Machine != IMAGE_FILE_MACHINE_AMD64)
	{
		panic(g_incorrectKernelImageArchitecture);
	}

	m_imageBase = KernelAllocator::getInstance().allocMemoryPageAlign(m_imageSize);
	m_imageBaseVirtual = reinterpret_cast<uintptr_t>(PagingManager64::getInstance().mapToKernel(m_imageBase, m_imageSize, false));
	const size_t peHdrEnd = dosHdr->e_lfanew + sizeof(*peHdr);
	kmemcpy(m_imageBase, moduleData.data(), peHdrEnd);

	auto sHdr = reinterpret_cast<PIMAGE_SECTION_HEADER>(&pbModule[peHdrEnd]);
	processPeSections(pbModule, moduleData.size(), peHdr, sHdr);
	initPeRelocation(peHdr);
	if (peHdr->OptionalHeader.AddressOfEntryPoint >= m_imageSize)
	{
		panic(g_incorrectKernelImageArchitecture);
	}
	m_entryPoint = m_imageBaseVirtual + peHdr->OptionalHeader.AddressOfEntryPoint;
}

void KernelLoader::processPeSections(void *fileBase, size_t fileSize, PIMAGE_NT_HEADERS peHeader, PIMAGE_SECTION_HEADER sections)
{
	size_t nSections = peHeader->FileHeader.NumberOfSections;
	size_t seg_align = peHeader->OptionalHeader.SectionAlignment;
	uint8_t* pbModule = static_cast<uint8_t*>(fileBase);
	uint8_t* pbImage = static_cast<uint8_t*>(m_imageBase);
	size_t vSize, vSizeRest;

	for (size_t i = 0; i < nSections; i++)
	{
		auto& section = sections[i];
		vSize = section.Misc.VirtualSize;
		vSizeRest = vSize % seg_align;
		if (vSizeRest > 0)
			vSize += seg_align - vSizeRest;
		if (section.VirtualAddress + vSize > m_imageSize)
		{
			panic(g_incorrectKernelFormatStr);
		}
		if (section.PointerToRawData != 0)
		{
			if (section.PointerToRawData + section.SizeOfRawData > fileSize)
			{
				panic(g_incorrectKernelFormatStr);
			}
			uint8_t* s_base = &pbImage[section.VirtualAddress];
			if (section.SizeOfRawData >= section.Misc.VirtualSize)
				kmemcpy(s_base, &pbModule[section.PointerToRawData], section.Misc.VirtualSize);
			else
			{
				kmemcpy(s_base, &pbModule[section.PointerToRawData], section.SizeOfRawData);
				kmemset(&s_base[section.SizeOfRawData], 0, section.Misc.VirtualSize - section.SizeOfRawData);
			}
		}
		else if (section.Misc.VirtualSize > 0)
			kmemset(&pbImage[section.VirtualAddress], 0, section.Misc.VirtualSize);

		// в ядре так делать нельзя
		if ((section.Characteristics & IMAGE_SCN_MEM_WRITE) == 0)
		{
			auto vAddr = m_imageBaseVirtual + section.VirtualAddress;
			PagingManager64::getInstance().protectPages(vAddr, vSize);
		}
	}
}

void KernelLoader::initPeRelocation(PIMAGE_NT_HEADERS ntHeader)
{
	uintptr_t relocOffset = ntHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress;
	if ((relocOffset == 0) || (ntHeader->OptionalHeader.ImageBase == m_imageBaseVirtual))
		return;

	const uintptr_t imageEndPhys = reinterpret_cast<uintptr_t>(m_imageBase);

	uintptr_t imageEnd = imageEndPhys + m_imageSize;
	size_t relocSize = ntHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size;
	uintptr_t deltaAddr = m_imageBaseVirtual - ntHeader->OptionalHeader.ImageBase;
	uintptr_t relocBase = reinterpret_cast<uintptr_t>(m_imageBase) + relocOffset;
	size_t max_reloc_size = m_imageSize - relocOffset;
	if (relocSize > max_reloc_size)
		relocSize = max_reloc_size;
	for (uintptr_t memory_end = relocBase + relocSize; relocBase < memory_end;)
	{
		auto& reloc = *reinterpret_cast<PIMAGE_BASE_RELOCATION>(relocBase);
		if (reloc.VirtualAddress == 0)
			break;
		const uintptr_t curBlkPtr = imageEndPhys + reloc.VirtualAddress;
		relocBase += sizeof(reloc);
		for (uintptr_t r_cnt = (reloc.SizeOfBlock - sizeof(reloc)) / 2; r_cnt-- > 0;)
		{
			uint16_t r = *reinterpret_cast<uint16_t*>(relocBase);
			uintptr_t curItemPtr = curBlkPtr + (r & 0x0FFF);
			if (curItemPtr >= (imageEnd - sizeof(uintptr_t)))
			{
				panic(g_incorrectKernelFormatStr);
			}
			switch (r >> 12)
			{
			case 0:
				break;

			case 3:
				*reinterpret_cast<uint32_t*>(curItemPtr) += static_cast<uint32_t>(deltaAddr);
				break;

			case 10:
				*reinterpret_cast<uint64_t*>(curItemPtr) += deltaAddr;
				break;

			default:
				panic(g_unknownRelocStr);
				break;
			}
			relocBase += 2;
		}
	}
}

void KernelLoader::run(KernelParams* params)
{
	params->m_imageBase = m_imageBaseVirtual;
	cpuJumpToKernel(m_entryPoint, m_stack + KERNEL_STACK_SIZE, reinterpret_cast<uintptr_t>(params));
}
