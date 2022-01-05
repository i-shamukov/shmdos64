/*
   IoResourceimpl.cpp
   Kernel IO resources manager
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

#include <kernel_export.h>
#include <cpu.h>
#include "IoResourceImpl.h"
#include <VirtualMemoryManager.h>

MmioSpace::MmioSpace(uintptr_t addr, size_t size)
	: m_ptr(static_cast<uint8_t*> (VirtualMemoryManager::system().mapMmio(addr, size)))
	, m_size(size)
{
}

MmioSpace::~MmioSpace()
{
	VirtualMemoryManager::system().unmapMmio(const_cast<uint8_t*>(m_ptr));
}

void MmioSpace::out8(uint64_t reg, uint8_t value)
{
	m_ptr[reg] = value;
}

void MmioSpace::out16(uint64_t reg, uint16_t value)
{
	*reinterpret_cast<volatile uint16_t*>(m_ptr + reg) = value;
}

void MmioSpace::out32(uint64_t reg, uint32_t value)
{
	*reinterpret_cast<volatile uint32_t*>(m_ptr + reg) = value;
}

void MmioSpace::out64(uint64_t reg, uint64_t value)
{
	*reinterpret_cast<volatile uint64_t*>(m_ptr + reg) = value;
}

uint8_t MmioSpace::in8(uint64_t reg)
{
	return m_ptr[reg];
}

uint16_t MmioSpace::in16(uint64_t reg)
{
	return *reinterpret_cast<volatile uint16_t*>(m_ptr + reg);
}

uint32_t MmioSpace::in32(uint64_t reg)
{
	return *reinterpret_cast<volatile uint32_t*>(m_ptr + reg);
}

uint64_t MmioSpace::in64(uint64_t reg)
{
	return *reinterpret_cast<volatile uint64_t*>(m_ptr + reg);
}

IoSpace::IoSpace(uint16_t base, uint16_t size)
	: m_base(base)
	, m_size(size)
{
}

void IoSpace::out8(uint64_t reg, uint8_t value)
{
	outportb(static_cast<uint16_t>(m_base + reg), value);
}

void IoSpace::out16(uint64_t reg, uint16_t value)
{
	outportw(static_cast<uint16_t>(m_base + reg), value);
}

void IoSpace::out32(uint64_t reg, uint32_t value)
{
	outportd(static_cast<uint16_t>(m_base + reg), value);
}

void IoSpace::out64(uint64_t reg, uint64_t value)
{
	outportd(static_cast<uint16_t>(m_base + reg), static_cast<uint32_t>(value & 0xFFFFFFFF));
	outportd(static_cast<uint16_t>(m_base + reg + 4), static_cast<uint32_t>(value >> 32));
}

uint8_t IoSpace::in8(uint64_t reg)
{
	return inportb(static_cast<uint16_t>(m_base + reg));
}

uint16_t IoSpace::in16(uint64_t reg)
{
	return inportw(static_cast<uint16_t>(m_base + reg));
}

uint32_t IoSpace::in32(uint64_t reg)
{
	return inportd(static_cast<uint16_t>(m_base + reg));
}

uint64_t IoSpace::in64(uint64_t reg)
{
	return static_cast<uint64_t> (inportd(static_cast<uint16_t>(m_base + reg))) | (static_cast<uint64_t>(inportd(static_cast<uint16_t>(m_base + reg + 4))) << 32);
}

KERNEL_SHARED IoResource* makeIoPortResource(uint16_t portBase, uint16_t size)
{
	return new IoSpace(portBase, size);
}

KERNEL_SHARED IoResource* makeMmioResource(uintptr_t physAddr, size_t size)
{
	return new MmioSpace(physAddr, size);
}
