/*
   LocalApic.h
   Kernel header
   SHM DOS64
   Copyright (c) 2022, Ilya Shamukov <ilya.shamukov@gmail.com>
   
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
#include <AbstractDevice.h>

class IoResource;
class LocalApic : public AbstractDevice
{
public:
	typedef unsigned int ApicCpuId;

public:
	void initCurrentCpu();
	void runCpu(ApicCpuId cpuId, uint16_t cs);
	static ApicCpuId getCpuId();
	static ApicCpuId systemCpuIdToApic(unsigned int cpuId);
	void sendIpi(ApicCpuId cpuId, uint8_t vector);
	void sendBroadcastIpi(uint8_t vector);
	void eoi();
	static LocalApic& system();

private:
	LocalApic();
	~LocalApic();
	LocalApic(const LocalApic&) = delete;
	LocalApic(LocalApic&&) = delete;
	void waitIpi() const;
	void sendCommand(ApicCpuId cpuId, uint32_t command);
	void sendInit(ApicCpuId cpuId);
	void sendStartup(ApicCpuId cpuId, uint8_t vector);

private:
	const uintptr_t m_mmioBase;
	IoResource* m_mmio = nullptr;
};
