/*
   kchrono.h
   Chrono implementation header for SHM DOS64
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
#include <chrono>
#include <kernel_export.h>
#include <common_types.h>

struct KERNEL_SHARED kernel_clock
{
      typedef std::chrono::nanoseconds duration;
      typedef duration::rep	rep;
      typedef duration::period period;
      typedef std::chrono::time_point<kernel_clock, duration> time_point;
      static constexpr bool is_steady = true;
      static time_point now();
      static time_point fast();
};

TimePoint KERNEL_SHARED getSystemClockNs100();
TimePoint KERNEL_SHARED TimePointFromMilliseconds(uint64_t milliseconds);
