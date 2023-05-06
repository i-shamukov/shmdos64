/*
   common_lib.cpp
   Kernel CRT
   SHM DOS64
   Copyright (c) 2023, Ilya Shamukov, ilya.shamukov@gmail.com
   
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

#include <cpu.h>
#include <kernel_export.h>
#include <kernel_params.h>
#include "EventObject.h"
#include "common_lib.h"
#include "AbstractTimer.h"

KERNEL_SHARED int krand()
{
	static unsigned int next = static_cast<unsigned int>(cpuReadTSC());
	next = next * 1103515245 + 12345;
	return (unsigned int)(next / 65536) % K_RAND_MAX;
}

KERNEL_SHARED void sleepMs(TimePoint delayMs)
{
	EventObject().wait(AbstractTimer::system()->fromMilliseconds(delayMs));
}

KERNEL_SHARED void sleepUs(TimePoint delayUs)
{
	AbstractTimer* timer = AbstractTimer::system();
	if (delayUs >= 1000)
	{
		EventObject().wait(timer->fromMicroseconds(delayUs));
	}
	else
	{
		TimePoint endTime = timer->timepoint() + timer->fromMicroseconds(delayUs);
		while (timer->timepoint() < endTime)
			cpuPause();
	}
}

KERNEL_SHARED uintptr_t acpiRsdpPhys()
{
	return getKernelParams()->m_acpiRsdpPhys;
}
