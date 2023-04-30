/*
   idt.cpp
   Kernel IDT routines
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
#include "idt.h"
#include "gdt.h"

#pragma pack(push, 1)

struct InterruptGate
{
	uint16_t m_offset0_15;
	uint16_t m_cs;
	uint8_t m_ist;
	uint8_t m_type;
	uint16_t m_offset16_31;
	uint32_t m_offset32_63;
	uint32_t m_reserved;
};
#pragma pack(pop)

static_assert(sizeof (InterruptGate) == 16);
static InterruptGate g_idt[256] = {};

namespace SystemIDT
{
	void install()
	{
		static const CpuIdtPointer idtPointer{sizeof (g_idt), &g_idt};
		cpuLoadIDT(idtPointer);
	}

	void setHandler(uint8_t vector, InterruptHandler handler, bool disableInterrupts)
	{
		const uintptr_t addr = reinterpret_cast<uintptr_t> (handler);
		InterruptGate& gate = g_idt[vector];
		gate.m_offset0_15 = static_cast<uint16_t> (addr & 0xFFFF);
		gate.m_offset16_31 = static_cast<uint16_t> ((addr >> 16) & 0xFFFF);
		gate.m_offset32_63 = static_cast<uint32_t> ((addr >> 32) & 0xFFFFFFFF);
		gate.m_cs = SYSTEM_CODE_SEGMENT;
		gate.m_ist = 0;
		gate.m_type = (disableInterrupts ? 0x8E : 0x8F);
		gate.m_reserved = 0;
	}
}
