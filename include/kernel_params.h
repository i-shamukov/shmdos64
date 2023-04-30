/*
   kernel_params.h
   Boot params header for SHM DOS64
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

enum : uint64_t
{
    RAM_VIRTUAL_BASE = 0xFFFF800000000000ULL,
    KERNEL_VIRTUAL_BASE = 0xFFFFFF8000000000ULL,
    KERNEL_STACK_SIZE = 0x10000ULL,
    KERNEL_MIN_VIRTUAL_MEMORY = (128ULL << 20),
    KERNEL_MAX_VIRTUAL_MEMORY = (511ULL << 30)
};

struct KernelParams
{
    struct Video
    {
        int m_screenWidth;
        int m_screenHeight;
        int m_screenPitch;
        int m_curTextPosX;
        int m_curTextPosY;
        size_t m_consoleBufferPos;
        size_t m_consoleBufferSize;
        char* m_consoleBuffer;
        uint8_t* m_bootFontBitmap;
        int m_bootFontWidth;
        int m_bootFontHeight;
        void* m_sreenBuffer;
    } m_video;
    
    struct PhysicalMemory
    {
        struct PhysicalMemoryRegion
        {
            uintptr_t m_start;
            uintptr_t m_end;
        };
        PhysicalMemoryRegion* m_physicalMemoryRegions;
        size_t m_regions;
    } m_physicalMemory;
    
    struct VirtualMemory
    {
        uintptr_t m_freeStart;
    } m_virtualMemory;
    
    struct Modules
    {
        struct BootModule
        {
            wchar_t m_fileName[260];
            void* m_data;
            size_t m_size;
        };
        BootModule* m_modules;
        size_t m_count;
    } m_modules;

    uintptr_t m_stackBase;
    void* m_acpiRsdp;
	uintptr_t m_imageBase; 
};

extern KernelParams* getKernelParams();