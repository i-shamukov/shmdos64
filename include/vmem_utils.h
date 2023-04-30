/*
   vmem_utils.h
   Virtual to physical address translation header for SHM DOS64
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
#include <kernel_params.h>

template<typename T>
static inline T* physToVirtual(T* ptr)
{
    return reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(ptr) + RAM_VIRTUAL_BASE);
}

template<typename T>
static inline T* physToVirtualInt(uintptr_t addr)
{
    return reinterpret_cast<T*>(addr + RAM_VIRTUAL_BASE);
}

template<typename T>
static inline uintptr_t virtualToPhysInt(T* ptr)
{
    return reinterpret_cast<uintptr_t>(ptr) - RAM_VIRTUAL_BASE;
}