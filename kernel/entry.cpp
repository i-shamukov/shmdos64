/*
   entry.cpp
   Kernel entry point & CRT
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

#include <kernel_export.h>
#include <kernel_params.h>
#include <kclib.h>
#include "panic.h"

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

KernelParams* g_kernelParams = nullptr;
KernelParams* getKernelParams()
{
	return g_kernelParams;
}

void KernelMain();

void entry(KernelParams* params)
{
	__do_global_ctors();
	g_kernelParams = params;
	KernelMain();
}


extern "C" KERNEL_SHARED int atexit(void (*)())
{
	return 0;
}

extern "C" KERNEL_SHARED  void ___chkstk_ms()
{

}

extern "C" KERNEL_SHARED  void* memset(void* ptr, int value, size_t size)
{
	return kmemset(ptr, value, size);
}

extern "C" KERNEL_SHARED void __cxa_pure_virtual()
{
	PANIC(L"Pure virtual call");
}

namespace std
{
	KERNEL_SHARED void __throw_bad_function_call()
	{
		PANIC(L"bad function call");
	}
}

