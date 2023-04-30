/*
   SmpBoot.cpp
   Kernel non-boot CPU startup code
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

#include "SmpBoot.h"
#include "gdt.h"

#define ROUTINE_INT_TO_STR_HELPER(value) #value
#define ROUTINE_INT_TO_STR(value) ROUTINE_INT_TO_STR_HELPER(value)

asm volatile(
	".globl smpBootLoaderStart\n"
	"smpBootLoaderStart:\n"
	".code16\n"
	"cli\n"
	"lgdt [" ROUTINE_INT_TO_STR(SMP_BOOT_GDT_POINTER_ADDR) "]\n"
	"mov EAX, CR4\n"
	"or EAX, 0x20\n"
	"mov CR4, EAX\n"
	"mov ECX, 0xC0000080\n"
	"rdmsr\n"
	"or EAX, 0x100\n"
	"wrmsr\n"
	"mov EAX, " ROUTINE_INT_TO_STR(SMP_BOOT_PAGE_TABLE_ROOT) "\n"
	"mov CR3, EAX\n"
	"mov EAX, CR0\n"
	"or EAX, 0x80000001\n"
	"mov CR0, EAX\n"
	".code32\n"
	".byte 0x66\n"
	"ljmp " ROUTINE_INT_TO_STR(SYSTEM_CODE_SEGMENT_MACRO) ":(smp64bitCodeStart - smpBootLoaderStart + " ROUTINE_INT_TO_STR(SMP_BOOT_CODE_BASE) ")\n"
	".code64\n"
	"smp64bitCodeStart:\n"
	"mov AX, " ROUTINE_INT_TO_STR(SYSTEM_DATA_SEGMENT_MACRO) "\n"
	"mov SS, AX\n"
	"mov RSP, [" ROUTINE_INT_TO_STR(SMP_BOOT_STACK_TOP) "]\n"
	"mov RCX, [" ROUTINE_INT_TO_STR(SMP_BOOT_ENTRY_ARG) "]\n"
	"jmp [" ROUTINE_INT_TO_STR(SMP_BOOT_ENTRY_POINT) "]\n"
	".globl smpBootLoaderEnd\n"
	"smpBootLoaderEnd:"	
);
