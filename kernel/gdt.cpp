/*
   gdt.cpp
   Kernel GDT routines
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

#include <kclib.h>
#include <cpu.h>
#include <kernel_params.h>
#include "smp.h"
#include "gdt.h"
#include "bootlib.h"

#pragma pack(push, 1)

struct GdtDescripor
{
	uint16_t m_limit0_15;
	uint16_t m_base0_15;
	uint8_t m_base16_23;
	uint8_t m_type8_15;
	uint8_t m_type16_23;
	uint8_t m_base24_31;
	uint32_t m_base32_63;
	uint32_t m_reserved;
};
#pragma pack(pop)

static_assert(sizeof (GdtDescripor) == 0x10);

static CpuGdtPointer initGDT()
{
	const uint16_t tableSize = (GDT_TABLE_DEFAULT_ITEMS_COUNT + MAX_CPU) * sizeof(GdtDescripor);
	GdtDescripor* table = static_cast<GdtDescripor*>(BootLib::virtualAlloc(tableSize, true));
	auto set = [table](int segment, uint8_t type8_15, uint8_t type16_23) {
		GdtDescripor& descriptor = table[segment / sizeof(GdtDescripor)];
		descriptor.m_type8_15 = type8_15;
		descriptor.m_type16_23 = type16_23 | 0xF;
		descriptor.m_limit0_15 = 0xFFFF;
	};

	set(SYSTEM_CODE_SEGMENT, 0x9A, 0xA0);
	set(SYSTEM_DATA_SEGMENT, 0x92, 0xC0);
	set(USER_CODE_SEGMENT, 0xFA, 0xA0);
	set(USER_DATA_SEGMENT, 0xF2, 0xC0);

	return CpuGdtPointer{tableSize, table};
}

static void initTssDescriptor(GdtDescripor& tssDescriptor, const SystemTaskSegmentState* tss)
{
	const uintptr_t tssAddr = reinterpret_cast<uintptr_t>(tss);
	static_assert(sizeof (*tss) < 0x10000);
	tssDescriptor.m_limit0_15 = sizeof (*tss);
	tssDescriptor.m_base16_23 = tssAddr & 0x00FFFFFF;
	tssDescriptor.m_base24_31 = (tssAddr >> 24) & 0xFF;
	tssDescriptor.m_base32_63 = tssAddr >> 32;
	tssDescriptor.m_type8_15 = 0xE9;
	tssDescriptor.m_type16_23 = 0;
}

const CpuGdtPointer& gdtPointerInstance()
{
	static const CpuGdtPointer gdtPointer = initGDT();
	return gdtPointer;
}

namespace SystemGDT
{
	void installOnBootCpu()
	{
		static SystemTaskSegmentState bootTss = {};
		install(BOOT_CPU_ID, &bootTss);
	}

	void install(unsigned int cpuId, const SystemTaskSegmentState* tss)
	{
		const CpuGdtPointer& gdtPointer = gdtPointerInstance();
		initTssDescriptor(gdtPointer.m_base[GDT_TABLE_DEFAULT_ITEMS_COUNT + cpuId], tss);
		cpuLoadGDT(gdtPointer);
		cpuLoadSS(SYSTEM_DATA_SEGMENT);
		cpuLoadCS(SYSTEM_CODE_SEGMENT);
		cpuLoadTaskRegister((GDT_TABLE_DEFAULT_ITEMS_COUNT + cpuId) * sizeof(GdtDescripor));
		cpuSetLocalPtr(LOCAL_CPU_TSS, tss);
	}
	
	void storeBootPart(void* gdt, void* pointer)
	{
		const size_t bootPartSize = GDT_TABLE_DEFAULT_ITEMS_COUNT * sizeof(GdtDescripor);
		const CpuGdtPointer& gdtPointer = gdtPointerInstance();
		kmemcpy(gdt, gdtPointer.m_base, bootPartSize);
		const CpuGdtPointer bootPointer = {bootPartSize, static_cast<GdtDescripor*>(gdt)};
		kmemcpy(pointer, &bootPointer, sizeof(bootPointer));
	}

	SystemTaskSegmentState& getTSS()
	{
		return *static_cast<SystemTaskSegmentState*>(cpuGetLocalPtr(LOCAL_CPU_TSS));
	}
}