/*
   PciLegacy.h
   Headers for PCI driver
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
#include <kmutex.h>
#include <IoResource.h>
#include <pci.h>

class PciLegacy : public IoResource
{
public:
	PciLegacy(IoResource* io, const PciAddress& pciAddr);
	~PciLegacy();
	void out8(uint64_t reg, uint8_t value) override;
	void out16(uint64_t reg, uint16_t value) override;
	void out32(uint64_t reg, uint32_t value) override;
	void out64(uint64_t reg, uint64_t value) override;
	uint8_t in8(uint64_t reg) override;
	uint16_t in16(uint64_t reg) override;
	uint32_t in32(uint64_t reg) override;
	uint64_t in64(uint64_t reg) override;
	
	static bool test(IoResource* io);

private:
	PciLegacy(const PciLegacy&) = delete;
	PciLegacy(PciLegacy&&) = delete;
	PciLegacy& operator=(const PciLegacy&) = delete;
	uint32_t setAddress(unsigned int reg);
	
private:
	IoResource* m_io;
	const uint32_t m_address;
	kmutex m_mutex;
};

