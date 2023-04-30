/*
   SmpBoot.h
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

#define SMP_BOOT_CODE_MAX_SIZE		0x0E00
#define SMP_BOOT_MEMORY_START		0x7000
#define SMP_BOOT_CODE_BASE			0x7000
#define SMP_BOOT_GDT_TABLE_ADDR		0x7E00
#define SMP_BOOT_GDT_POINTER_ADDR	0x7F00
#define SMP_BOOT_ENTRY_POINT		0x7F10
#define SMP_BOOT_STACK_TOP			0x7F18
#define SMP_BOOT_ENTRY_ARG			0x7F20
#define SMP_BOOT_PAGE_TABLE_ROOT	0x8000
#define SMP_BOOT_MEMORY_END			0x9000

extern "C" 
{
	void smpBootLoaderStart();
	void smpBootLoaderEnd();
};
