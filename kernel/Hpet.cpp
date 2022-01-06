/*
   Hpet.cpp
   Kernel HPET Driver
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

#include <IoResource.h>
#include <conout.h>
#include "AcpiTables.h"
#include "panic.h"
#include "Hpet.h"
#include "ExternalInterrupts.h"
#include "TaskManager.h"

static const size_t g_hpetMmioSpaceSize = 0x1000;
static const unsigned int hpetApicIrq = 2;

enum
{
	GeneralCapabilitiesReg = 0x00,
	GeneralConfigurationReg = 0x10,
	MainCounterValueReg = 0xF0
};

enum
{
	TimerRegConfigurationAndCapability = 0x00,
	TimerRegComparator = 0x08
};

static inline int timerBaseReg(int timer)
{
	return timer * 0x20 + 0x100;
}

Hpet::Hpet()
	: AbstractDevice(DeviceClass::System, L"HPET", AbstractDevice::root())
	, m_mmio(io(AcpiTables::instance().hpetMmioBase(), g_hpetMmioSpaceSize, IoResourceType::MmioSpace))
{
	static const uint64_t legRtCap = 0x8000;

	static const uint64_t tmPeriodicCap = (1 << 4);

	static const uint32_t tmIntEnable = (1 << 2);
	static const uint32_t tmTypePeriodic = (1 << 3);
	static const uint32_t tmValSetCfg = (1 << 6);

	static const uint64_t enableCfg = 0x01;
	static const uint64_t legRtCfg = 0x02;

	const uint64_t cap = m_mmio->in64(GeneralCapabilitiesReg);
	const uint64_t tickPeriod = (cap >> 32);
	if (tickPeriod < 1000000)
		PANIC(L"Invalid HPET tick period");

	if ((cap & legRtCap) == 0)
		PANIC(L"HPET isn't supports the Legacy Replacement Interrupt Route option");

	const uint64_t freq = (1000000000000000ULL + (tickPeriod / 2)) / tickPeriod;
	println(L"HPET frequency ", freq, L"Hz");
	AbstractTimer::setFrequency(freq);
	int numTimers = (cap >> 8) & 0x1F;
	println(L"HPET capabilities: ", hex(cap, false));

	m_timerRegBase = 0;
	for (int timer = 0; timer < numTimers; ++timer)
	{
		m_timerRegBase = timerBaseReg(timer);
		const uint64_t timerCap = m_mmio->in64(m_timerRegBase + TimerRegConfigurationAndCapability);
		if ((timerCap & tmPeriodicCap) != 0)
		{
			println(L"HPET timer ", timer, L" capabilities: ", hex(timerCap, false));
			break;
		}
	}
	if (m_timerRegBase == 0)
		PANIC(L"HPET isn't support periodic mode or 32 bit");

	m_mmio->out64(MainCounterValueReg, 0);
	m_mmio->out32(m_timerRegBase + TimerRegConfigurationAndCapability, tmValSetCfg | tmTypePeriodic | tmIntEnable);
	m_mmio->out64(m_timerRegBase + TimerRegComparator, freq / SYSTEM_TIMER_FREQUENCY_DIV);
	m_mmio->out64(GeneralConfigurationReg, enableCfg | legRtCfg);
}

Hpet::~Hpet()
{
	PANIC(L"not implemented");
}

TimePoint Hpet::timepoint() const
{
	return m_mmio->in64(MainCounterValueReg);
}

void Hpet::install()
{
	static Hpet hpet;
	AbstractTimer::setSystemTimer(&hpet);
}

void Hpet::setEnableInterrupt(bool enable)
{
	static const uint32_t tmIntEnable = (1 << 2);
	uint32_t conf = m_mmio->in32(m_timerRegBase + TimerRegConfigurationAndCapability);
	if (enable)
	{
		conf |= tmIntEnable;
		installInterruptHandler(hpetApicIrq);
	}
	else
	{
		conf &= ~tmIntEnable;
		removeInterruptHandler();
	}
	m_mmio->out32(m_timerRegBase + TimerRegConfigurationAndCapability, conf);
}

bool Hpet::interruptHandler()
{
	eoi();
	AbstractTimer::onInterrupt();
	return true;
}
