/*
   Semaphore.cpp
   Kernel semaphore
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

#include <algorithm>
#include "ThreadLocalStorage.h"
#include "Semaphore.h"

ksem::ksem(uint32_t initialValue, uint32_t max)
	: m_private(new Semaphore(initialValue, max))
{

}

ksem::~ksem()
{
	delete m_private;
}

bool ksem::wait(uint32_t count, TimePoint timeout)
{
	return m_private->wait(count, timeout);
}

void ksem::signal(uint32_t count, uint32_t* oldCount)
{
	return m_private->signal(count, oldCount);
}

Semaphore::Semaphore(uint32_t initialValue, uint32_t max)
	: m_state(State{initialValue, 0})
	, m_max(max)
{
	static_assert(m_state.is_always_lock_free);
}

template<bool incWaitCount>
bool Semaphore::waitUpdate(State& state, uint32_t count)
{
	for (;;)
	{
		if (state.m_value < count)
		{
			const uint32_t rest = count - state.m_value;
			State newState{0, state.m_waitCount};
			if constexpr (incWaitCount)
			{
				++newState.m_waitCount;
			}
			if (m_state.compare_exchange_strong(state, newState, std::memory_order_acquire, std::memory_order_relaxed))
			{
				threadSetLocalData(LOCAL_THREAD_STORAGE_SEM_VALUE, rest);
				return false;
			}
		}
		else
		{
			const State newState{state.m_value - count, state.m_waitCount};
			if (m_state.compare_exchange_strong(state, newState, std::memory_order_acquire, std::memory_order_relaxed))
				return true;
		}
	}
}

bool Semaphore::wait(uint32_t count, TimePoint timeout)
{
	if (count == 0)
		return true;

	if (count > m_max)
		return false;

	State state = m_state.load(std::memory_order_acquire);
	if (timeout == 0)
	{
		if (state.m_value < count)
			return false;

		const State newState{state.m_value - count, state.m_waitCount};
		return m_state.compare_exchange_strong(state, newState, std::memory_order_acquire, std::memory_order_relaxed);
	}

	if (waitUpdate<false>(state, count))
		return true;

	return EventObject::wait(timeout);
}

bool Semaphore::onWaitBegin()
{
	const uint32_t count = static_cast<uint32_t>(threadGetLocalData(LOCAL_THREAD_STORAGE_SEM_VALUE));
	State state = m_state.load(std::memory_order_acquire);
	return !waitUpdate<true>(state, count);
}

void Semaphore::signal(uint32_t count, uint32_t* oldCount)
{
	if (count == 0)
		return;

	State state = m_state.load(std::memory_order_acquire);
	for (;;)
	{
		const State newState{std::min(state.m_value + count, m_max), state.m_waitCount};
		if (oldCount)
			*oldCount = state.m_value;
		if (m_state.compare_exchange_strong(state, newState, std::memory_order_acquire, std::memory_order_relaxed))
		{
			state = newState;
			break;
		}
	}

	while ((state.m_waitCount != 0) && (state.m_value > 0))
	{
		EventObject::set();
		state = m_state.load(std::memory_order_acquire);
	}
}

bool Semaphore::onWakeSingleThread(Task* task)
{
	uint32_t count = getTaskTlsValue(task, LOCAL_THREAD_STORAGE_SEM_VALUE);
	State state = m_state.load(std::memory_order_acquire);
	while (state.m_value > 0)
	{
		if (state.m_value < count)
		{
			const uint32_t rest = count - state.m_value;
			const State newState{0, state.m_waitCount};
			if (m_state.compare_exchange_strong(state, newState, std::memory_order_acquire, std::memory_order_relaxed))
			{
				setTaskTlsValue(task, LOCAL_THREAD_STORAGE_SEM_VALUE, rest);
				return false;
			}
		}
		else
		{
			const State newState{state.m_value - count, state.m_waitCount - 1};
			if (m_state.compare_exchange_strong(state, newState, std::memory_order_acquire, std::memory_order_relaxed))
				return true;
		}
	}
	return false;
}
