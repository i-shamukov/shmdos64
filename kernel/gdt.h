/*
   gdt.h
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

#define SYSTEM_CODE_SEGMENT_MACRO 0x10
#define SYSTEM_DATA_SEGMENT_MACRO 0x20

enum : uint16_t
{
	SYSTEM_CODE_SEGMENT = SYSTEM_CODE_SEGMENT_MACRO,
	SYSTEM_DATA_SEGMENT = SYSTEM_DATA_SEGMENT_MACRO,
	USER_CODE_SEGMENT = 0x33,
	USER_DATA_SEGMENT = 0x43,
};

enum : size_t
{
	GDT_TABLE_DEFAULT_ITEMS_COUNT = 5
};

#pragma pack(push, 1)

struct SystemTaskSegmentState
{
	uint32_t m_reserved_1;
	uint64_t m_rsp[3];
	uint32_t m_reserved_2[2];
	uint64_t m_ist[7];
	uint32_t m_reserved_3[2];
	uint16_t m_reserved_4;
	uint16_t m_ioMapBase;
};
#pragma pack(pop)

static_assert(sizeof (SystemTaskSegmentState) == 104);

namespace SystemGDT
{
	void installOnBootCpu();
	void install(unsigned int cpuId, const SystemTaskSegmentState* tss);
	void storeBootPart(void* gdt, void* pointer);
	SystemTaskSegmentState& getTSS();
}
