/*
   IoResourceImpl.h
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
#include <IoResource.h>

class MmioSpace : public IoResource
{
public:
	MmioSpace(uintptr_t addr, size_t size);
	~MmioSpace();
	void out8(uint64_t reg, uint8_t value) override;
	void out16(uint64_t reg, uint16_t value) override;
	void out32(uint64_t reg, uint32_t value) override;
	void out64(uint64_t reg, uint64_t value) override;
	uint8_t in8(uint64_t reg) override;
	uint16_t in16(uint64_t reg) override;
	uint32_t in32(uint64_t reg) override;
	uint64_t in64(uint64_t reg) override;
	
	volatile uint8_t* ptr() const
	{
		return m_ptr;
	}

private:
	MmioSpace(const MmioSpace&) = delete;
	MmioSpace(MmioSpace&&) = delete;

private:
	volatile uint8_t* const m_ptr;
	const size_t m_size;
};

class IoSpace : public IoResource
{
public:
	IoSpace(uint16_t addr, uint16_t size);
	void out8(uint64_t reg, uint8_t value) override;
	void out16(uint64_t reg, uint16_t value) override;
	void out32(uint64_t reg, uint32_t value) override;
	void out64(uint64_t reg, uint64_t value) override;
	uint8_t in8(uint64_t reg) override;
	uint16_t in16(uint64_t reg) override;
	uint32_t in32(uint64_t reg) override;
	uint64_t in64(uint64_t reg) override;

private:
	IoSpace(const MmioSpace&) = delete;
	IoSpace(MmioSpace&&) = delete;

private:
	const uint16_t m_base;
	const uint16_t m_size;
};
