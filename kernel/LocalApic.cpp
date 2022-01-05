/*
   LocalApic.cpp
   Kernel Local APIC routines
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

#include <cpu.h>
#include <IoResource.h>
#include "idt.h"
#include "panic.h"
#include "smp.h"
#include "common_lib.h"
#include "IoResourceImpl.h"
#include "LocalApic.h"

enum
{
	ApicIdReg = 0x020,
	ApicEoiReg = 0x0B0,
	SpuriousInterruptVectorReg = 0x0F0,
	InterruptCommandLo = 0x300,
	InterruptCommandHi = 0x310,
};

enum
{
	apicIcrModeFixed = (0 << 8),
	apicIcrModeInit = (5 << 8),
	apicIcrModeStartUp = (6 << 8),
	apicIcrModeAll = (3 << 18),
	apicIcrStatusPending = (1 << 12),
	apicIcrLevelAssert = (1 << 14)
};

#define APIC_NULL_HANDLER(procName)\
	asm volatile(".globl " #procName);\
	asm volatile(#procName ":");\
	asm volatile("iret");\
	extern "C" void procName();\
	
APIC_NULL_HANDLER(apicSpuriousInterrupt)

static LocalApic::ApicCpuId g_systemCpuIdToApicMap[MAX_CPU] = {};

LocalApic::LocalApic()
	: AbstractDevice(DeviceClass::System, L"Local APIC", AbstractDevice::root())
	, m_mmioBase(cpuAlignAddrLo(cpuReadMSR(CPU_MSR_APIC_BASE) & cpuMaxPhysAddr()))
{
	if (m_mmioBase == 0)
		PANIC(L"Null pointer to LAPIC");

	m_mmio = io(m_mmioBase, PAGE_SIZE, IoResourceType::MmioSpace);
	cpuSetLocalPtr(LOCAL_CPU_APIC_EOI_ADDR, const_cast<uint8_t*>(static_cast<MmioSpace*>(m_mmio)->ptr()) + ApicEoiReg);
	SystemIDT::setHandler(CPU_APIC_SPURIOUS_VECTOR, &apicSpuriousInterrupt, false);
}

LocalApic::~LocalApic()
{
	PANIC(L"not implemented");
}

LocalApic& LocalApic::system()
{
	static LocalApic apic;
	return apic;
}

void LocalApic::initCurrentCpu()
{
	const uint64_t apicBaseEnable = 0x800;
	const uint32_t apicSoftwareEnable = 0x100;
	cpuWriteMSR(CPU_MSR_APIC_BASE, m_mmioBase | apicBaseEnable);
	m_mmio->out32(SpuriousInterruptVectorReg, CPU_APIC_SPURIOUS_VECTOR | apicSoftwareEnable);
	const ApicCpuId apicId = m_mmio->in32(ApicIdReg) >> 24;
	if (apicId >= MAX_CPU)
		PANIC(L"Too long APIC ID");

	g_systemCpuIdToApicMap[cpuCurrentId()] = apicId;
	cpuSetLocalData(LOCAL_CPU_APIC_ID, apicId);
	cpuSetLocalPtr(LOCAL_CPU_APIC_EOI_ADDR, const_cast<uint8_t*>(static_cast<MmioSpace*>(m_mmio)->ptr()) + ApicEoiReg);
}

LocalApic::ApicCpuId LocalApic::getCpuId()
{
	return static_cast<LocalApic::ApicCpuId>(cpuGetLocalData(LOCAL_CPU_APIC_ID));
}

LocalApic::ApicCpuId LocalApic::systemCpuIdToApic(unsigned int cpuId)
{
	return g_systemCpuIdToApicMap[cpuId];
}

void LocalApic::waitIpi() const
{
	while ((m_mmio->in32(InterruptCommandLo) & apicIcrStatusPending) != 0)
		cpuPause();
}

void LocalApic::sendCommand(ApicCpuId cpuId, uint32_t command)
{
	const uint32_t cpuIdShift = 24;
	waitIpi();
	m_mmio->out32(InterruptCommandHi, (static_cast<uint32_t>(cpuId) << cpuIdShift));
	m_mmio->out32(InterruptCommandLo, command);
}

void LocalApic::sendInit(ApicCpuId cpuId)
{
	sendCommand(cpuId, apicIcrModeInit | apicIcrLevelAssert);
}

void LocalApic::sendStartup(ApicCpuId cpuId, uint8_t vector)
{
	sendCommand(cpuId, apicIcrModeStartUp | apicIcrLevelAssert | static_cast<uint32_t>(vector));
}

void LocalApic::runCpu(ApicCpuId cpuId, uint16_t cs)
{
	const uint8_t vector = static_cast<uint8_t>(cs >> 8);
	sendInit(cpuId);
	sleepMs(10);
	sendStartup(cpuId, vector);
	sleepUs(200);
	sendStartup(cpuId, vector);
}

void LocalApic::sendIpi(ApicCpuId cpuId, uint8_t vector)
{
	sendCommand(cpuId, apicIcrModeFixed | apicIcrLevelAssert | static_cast<uint32_t>(vector));
}

void LocalApic::sendBroadcastIpi(uint8_t vector)
{
	sendCommand(0xFF, apicIcrModeAll | apicIcrModeFixed | apicIcrLevelAssert | static_cast<uint32_t>(vector));
}

void LocalApic::eoi()
{
	m_mmio->out32(ApicEoiReg, 0);
}
