/*
   entry.cpp
   EFI entry point & CRT routines
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

#include "efi.h"

typedef void (*func_ptr) (void);
extern func_ptr __CTOR_LIST__[];

static void __do_global_ctors(void)
{
	__SIZE_TYPE__ nptrs = (__SIZE_TYPE__)__CTOR_LIST__[0];
	if (nptrs == (__SIZE_TYPE__)-1)
	{
		for (nptrs = 0; __CTOR_LIST__[nptrs + 1] != 0; ++nptrs);
	}
	for (unsigned int i = nptrs; i >= 1; --i)
	{
		__CTOR_LIST__[i]();
	}
}

const EFI_SYSTEM_TABLE* g_systemTable = nullptr;
EFI_HANDLE g_imageHandle = nullptr;

const EFI_SYSTEM_TABLE* getSystemTable()
{
	return g_systemTable;
}

EFI_HANDLE getImageHandle()
{
	return g_imageHandle;
}

void EfiMain();

EFI_STATUS EFIAPI entry(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE* SystemTable)
{
	__do_global_ctors();
	g_imageHandle = ImageHandle;
	g_systemTable = SystemTable;
	EfiMain();
	return EFI_SUCCESS;
}

extern "C" int atexit(void (*)())
{
	return 0;
}
