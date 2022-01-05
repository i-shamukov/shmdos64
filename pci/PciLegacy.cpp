/*
   PciLegacy.cpp
   PCI bus implementaion for legacy mechanism
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

#include "PciLegacy.h"

enum
{
    PCI_ADDR_PORT = 0,
    PCI_DATA_PORT = 4
};

PciLegacy::PciLegacy(IoResource* io, const PciAddress& pciAddr)
	: m_io(io)
	, m_address( ((pciAddr.m_bus & 0xFF) << 16) | ((pciAddr.m_device & 0x1F)  << 11) | ((pciAddr.m_function & 0x07) << 8) )
{

}

bool PciLegacy::test(IoResource* io)
{
	io->out32(PCI_ADDR_PORT, 0x80000000);
    return (io->in32(PCI_ADDR_PORT) == 0x80000000);
}

PciLegacy::~PciLegacy()
{
	
}

uint32_t PciLegacy::setAddress(unsigned int reg)
{
	const uint32_t addr = m_address | (reg & 0xFC);
	m_io->out32(PCI_ADDR_PORT, addr);
	return addr;
}

void PciLegacy::out8(uint64_t reg, uint8_t value)
{
	klock_guard lock(m_mutex);
	setAddress(reg);
    uint32_t oldValue = m_io->in32(PCI_DATA_PORT);
    uint32_t newValue = value;
    const uint32_t shift = ( (reg & 0x03) * 8 );
    newValue <<= shift;
    oldValue &= ~(0xFF << shift); 
    newValue |= oldValue; 
    m_io->out32(PCI_DATA_PORT, newValue);
}

void PciLegacy::out16(uint64_t reg, uint16_t value)
{
	klock_guard lock(m_mutex);
	setAddress(reg);
	uint32_t oldValue = m_io->in32(PCI_DATA_PORT);
	uint32_t newValue = value;
	const uint32_t shift = ( (reg & 0x01) * 16 );
	newValue <<= shift;
	oldValue &= ~(0xFFFF << shift); 
	newValue |= oldValue;
	m_io->out32(PCI_DATA_PORT, newValue);
}

void PciLegacy::out32(uint64_t reg, uint32_t value)
{
	klock_guard lock(m_mutex);
	setAddress(reg);
	m_io->out32(PCI_DATA_PORT, value);
}

void PciLegacy::out64(uint64_t reg, uint64_t value)
{
	klock_guard lock(m_mutex);
	const uint32_t addr = setAddress(reg);
	m_io->out32(PCI_DATA_PORT, value & 0xFFFFFFFF);
	m_io->out32(PCI_ADDR_PORT, addr + 4);
	m_io->out32(PCI_DATA_PORT, value >> 32);
}

uint8_t PciLegacy::in8(uint64_t reg)
{
	klock_guard lock(m_mutex);
	setAddress(reg);
	return static_cast<uint8_t>((m_io->in32(PCI_DATA_PORT) >> ((reg & 0x03) * 8 )) & 0xFF);
}

uint16_t PciLegacy::in16(uint64_t reg)
{
	klock_guard lock(m_mutex);
	setAddress(reg);
	return static_cast<uint16_t>((m_io->in32(PCI_DATA_PORT) >> ((reg & 0x01) * 16 )) & 0xFFFF);
}

uint32_t PciLegacy::in32(uint64_t reg)
{
	klock_guard lock(m_mutex);
	setAddress(reg);
	return m_io->in32(PCI_DATA_PORT);
}

uint64_t PciLegacy::in64(uint64_t reg)
{
	klock_guard lock(m_mutex);
	const uint32_t addr = setAddress(reg);
	uint64_t value = m_io->in32(PCI_DATA_PORT);
	m_io->out32(PCI_ADDR_PORT, addr + 4);
	value |= (static_cast<uint64_t>(m_io->in32(PCI_DATA_PORT)) << 32);
	return value;
}
	
