/*
   smp.h
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
#include <cpu.h>

#define LOCAL_CPU_MT_LOCK_COUNT_MACRO 0x048
#define LOCAL_CPU_NEED_TASK_SWITCH_MACRO 0x050
#define LOCAL_CPU_APIC_EOI_ADDR_MACRO 0x060

enum : uint64_t
{
	LOCAL_CPU_ID = 0x000,
	LOCAL_CPU_PAGING_MGR = 0x008,
	LOCAL_CPU_TSS = 0x010,
	LOCAL_CPU_APIC_ID = 0x018,
	LOCAL_CPU_IDLE_TASK = 0x020,
	LOCAL_CPU_SHED_TIME = 0x028,
	LOCAL_CPU_SPIN_DATA = 0x030,
	LOCAL_CPU_NEXT_TASK = 0x038,
	LOCAL_CPU_TS_FLAG = 0x040,
	LOCAL_CPU_MT_LOCK_COUNT = LOCAL_CPU_MT_LOCK_COUNT_MACRO,
	LOCAL_CPU_NEED_TASK_SWITCH = LOCAL_CPU_NEED_TASK_SWITCH_MACRO,
	LOCAL_CPU_TLB_TASK = 0x58,
	LOCAL_CPU_APIC_EOI_ADDR = LOCAL_CPU_APIC_EOI_ADDR_MACRO,
	LOCAL_CPU_DATA_SIZE = PAGE_SIZE
};

namespace SystemSMP
{
	void initBootCpu();
	void initCpu(unsigned int cpuId, void* localCpuData, void* localCpuSpinsData);
	void init();
	bool isInit();
};

