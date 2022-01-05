/*
   BootIo.h
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
#include "efi.h"
#include <common_types.h>
#include <kvector.h>

enum : size_t
{
	MAX_PATH = 260
};

class BootIo
{
public:
	static BootIo& getInstance();
	bool readFile(const wchar_t* wFileName, size_t maxSize, kvector<char>* dataPtr, void** pagesPtr = nullptr, size_t* dataSize = nullptr);
	bool writeFile(const wchar_t* wFileName, const char* data, size_t size);

private:
	BootIo();
	BootIo(const BootIo&) = delete;
	BootIo(BootIo&&) = delete;

private:
	EFI_FILE_PROTOCOL* m_sysFolder;
};

