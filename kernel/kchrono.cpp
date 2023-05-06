/*
   kchrono.cpp
   Chrono implementation for SHM DOS64
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

#include <kchrono.h>
#include "AbstractTimer.h"

kernel_clock::time_point kernel_clock::now()
{
    const AbstractTimer* timer = AbstractTimer::system();
    return time_point(duration(std::chrono::nanoseconds(timer->toNs100(timer->timepoint()) * 100)));
}

kernel_clock::time_point kernel_clock::fast()
{
    const AbstractTimer* timer = AbstractTimer::system();
    return time_point(duration(std::chrono::nanoseconds(timer->toNs100(timer->fastTimepoint()) * 100)));
}

TimePoint getSystemClockNs100()
{
    const AbstractTimer* timer = AbstractTimer::system();
    return timer->toNs100(timer->fastTimepoint());
}

TimePoint TimePointFromMilliseconds(uint64_t milliseconds)
{
    return AbstractTimer::system()->fromMilliseconds(milliseconds);
}