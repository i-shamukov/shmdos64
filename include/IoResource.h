/*
   IoResource.h
   Kernel shared header 
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
#include <MemoryType.h>

class IoResource
{
public:
	virtual ~IoResource() { }
	virtual void out8(uint64_t reg, uint8_t value) = 0;
	virtual void out16(uint64_t reg, uint16_t value) = 0;
	virtual void out32(uint64_t reg, uint32_t value) = 0;
	virtual void out64(uint64_t reg, uint64_t value) = 0;
	virtual uint8_t in8(uint64_t reg) = 0;
	virtual uint16_t in16(uint64_t reg) = 0;
	virtual uint32_t in32(uint64_t reg) = 0;
	virtual uint64_t in64(uint64_t reg) = 0;
};

IoResource* makeIoPortResource(uint16_t portBase, uint16_t size);
IoResource* makeMmioResource(uintptr_t physAddr, size_t size, MemoryType memoryType = MemoryType::CacheDisabled);
