/*
   Semaphore.h
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
#include "EventObject.h"

class Semaphore : private EventObject
{
public:
	static const TimeStamp WaitInfinite = EventObject::WaitInfinite;
	Semaphore(uint32_t initialValue, uint32_t max);
	bool wait(uint32_t count, TimeStamp timeout);
	void signal(uint32_t count, uint32_t* oldCount);

private:
	struct State
	{
		uint32_t m_value;
		uint32_t m_waitCount;
	};

private:
	bool onWaitBegin() override;
	bool onWakeSingleThread(Task* task) override;
	template<bool incWaitCount>
	bool waitUpdate(State& state, uint32_t count);

private:
	std::atomic<State> m_state;
	const uint32_t m_max;
};
