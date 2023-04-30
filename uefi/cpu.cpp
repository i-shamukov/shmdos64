/*
   cpu.cpp
   CPU routines for bootloader
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
#include <conout.h>
#include "UefiVideo.h"
#include "panic.h"

void dumpControlRegisters()
{
	println(L"Dump control registers:");
	println(L"CR0 = ", hex(cpuGetCR0(), false));
	println(L"CR3 = ", hex(cpuGetCR3(), false));
	println(L"CR4 = ", hex(cpuGetCR4(), false));
	println(L"CR8 = ", hex(cpuGetCR8(), false));
	println(L"PAT = ", hex(cpuReadMSR(CPU_MSR_IA32_PAT), false));

	const int vcnt = cpuReadMSR(CPU_MSR_IA32_MTRRCAP) & 0xFF;
	if (vcnt > 0)
	{
		println(L"MTRRs:");
		for (int idx = 0; idx < vcnt; ++idx)
		{
			const uint64_t addr = cpuReadMSR(CPU_MSR_IA32_MTRR_PHYSBASE0 + idx * 2);
			const uint64_t mask = cpuReadMSR(CPU_MSR_IA32_MTRR_PHYSMASK0 + idx * 2);
			println(hex(addr, false), L' ', hex(mask, false));
		}
	}
}

void fixFrameBufferMtrr()
{
	const int vcnt = cpuReadMSR(CPU_MSR_IA32_MTRRCAP) & 0xFF;
	const uintptr_t frameBuffer = UefiVideo::getInstance().frameBuffer();
	const uint64_t valid = 0x800;
	const uint64_t writeCombinig = 0x01;
	for (int idx = 0; idx < vcnt; ++idx)
	{
		const uint64_t addr = cpuReadMSR(CPU_MSR_IA32_MTRR_PHYSBASE0 + idx * 2) & ~PAGE_MASK;
		uint64_t mask = cpuReadMSR(CPU_MSR_IA32_MTRR_PHYSMASK0 + idx * 2);
		if ((mask & valid) == 0)
			continue;

		mask &= ~PAGE_MASK;
		if ((frameBuffer & mask) == addr)
		{
			print(L"Setup MTRR for Frame Buffer... ");
			const uint64_t cr0 = cpuGetCR0();
			const uint64_t cr4 = cpuGetCR4();
			cpuSetCR0(cr0 | CR0_CACHE_DISABLE);
			cpuInvalidateCache();

			cpuWriteMSR(CPU_MSR_IA32_MTRR_PHYSBASE0 + idx * 2, addr | writeCombinig);

			cpuInvalidateCache();
			cpuSetCR4(cr4 & ~CR4_PAGE_GLOBAL_ENABLE);
			cpuSetCR0(cr0);
			cpuSetCR4(cr4);
			println(L"OK");
			break;
		}
	}
}

void cpuTestCommands()
{
	uint32_t eax = CPUID_PROCESSOR_INFO_EAX;
	uint32_t ebx;
	uint32_t edx;
	uint32_t ecx;
	cpuCpuid(eax, ebx, edx, ecx);
	if ((ecx & CPUID_PROCESSOR_INFO_ECX_CMPXCHG16B) == 0)
		panic(L"CPU does not support CMPXCHG16B instruction");

	if (!cpuSupport1GbPages())
		println(L"CPU does not support 1GB pages");
}

bool cpuSupport1GbPages()
{
	static bool result = []() -> bool {
		uint32_t eax = CPUID_PROCESSOR_INFOEX_EAX;
		uint32_t ebx;
		uint32_t edx;
		uint32_t ecx;
		cpuCpuid(eax, ebx, edx, ecx);
		return ((edx & CPUID_PROCESSOR_INFOEX_EDX_1GBPAGES) != 0);
	}();
	return result;
}
