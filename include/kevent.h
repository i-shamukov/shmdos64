/*
   kevent.h
   Shared header for SHM DOS64
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
#include <limits>

class EventObject;
class kevent
{
public:
	static const TimeStamp WaitInfinite = std::numeric_limits<TimeStamp>::max();
	static const uint32_t WaitError = 0xFFFFFFFF;
	static const uint32_t WaitTimeout = 0x102;

public:
	kevent();
	kevent(bool set, bool manualReset);
	~kevent();
	void set();
	uint32_t waitExStatus(TimeStamp timeout);
	bool wait(TimeStamp timeout = WaitInfinite);
	static uint32_t waitMultiple(kevent** objects, uint32_t numObjects, bool waitAll, TimeStamp timeout);

private:
	kevent(const kevent&) = delete;
	kevent(kevent&&) = delete;
	kevent& operator=(const kevent&) = delete;

private:
	EventObject* m_private;
};
