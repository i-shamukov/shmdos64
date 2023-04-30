/*
   KernelLoader.h
   Header for EFI BootLoader
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
#include <kstring.h>
#include <kernel_params.h>
#include <kvector.h>
#include <pe64.h>

class KernelLoader
{
public:
	KernelLoader(const kwstring& kernelName, KernelParams* params);
	void run(KernelParams* params);

private:
	void loadFromMemory(kvector<char>& moduleData);
	void processPeSections(void *fileBase, size_t fileSize, PIMAGE_NT_HEADERS peHeader, PIMAGE_SECTION_HEADER sections);
	void initPeRelocation(PIMAGE_NT_HEADERS ntHeader);

private:
	size_t m_imageSize;
	void* m_imageBase;
	uintptr_t m_imageBaseVirtual;
	uintptr_t m_entryPoint;
	uintptr_t m_stack;
};
