/*
   kevent.h
   Shared header for SHM DOS64
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

#include <kevent.h>
#include <common_types.h>
#include <kernel_export.h>


class Semaphore;
class KERNEL_SHARED ksem
{
public:
	static const TimePoint WaitInfinite = kevent::WaitInfinite;
	static const uint32_t WaitError = kevent::WaitError;

public:
    ksem(uint32_t initialValue, uint32_t max);
    ~ksem();
    bool wait(uint32_t count, TimePoint timeout);
	void signal(uint32_t count, uint32_t* oldCount);

private:
	ksem(const ksem&) = delete;
	ksem(ksem&&) = delete;
	ksem& operator=(const ksem&) = delete;

private:
	Semaphore* m_private;
};