/*
   kmutex.cpp
   Kernel fast mutex
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

#include <limits>
#include <cpu.h>
#include "ThreadLocalStorage.h"
#include "panic.h"
#include "kmutex_p.h"

static const unsigned int g_waitCountStartValue = MAX_CPU;
static const unsigned int g_freeSpinValue = std::numeric_limits<unsigned int>::max();


MutexPrivate::MutexPrivate(int spinCount)
	: m_spinCount(spinCount)
	, m_lockCpu(g_freeSpinValue)
{

}

MutexPrivate::~MutexPrivate()
{
	if (m_lockCpu.load(std::memory_order_relaxed) != g_freeSpinValue)
		PANIC(L"destroying busy mutex");
}

unsigned int MutexPrivate::onWake()
{
	unsigned int lock = m_lockCpu.load(std::memory_order_acquire);
	for (;;)
	{
		const unsigned int newLock = ((lock == g_waitCountStartValue) ? cpuCurrentId() : (lock - 1));
		if (m_lockCpu.compare_exchange_strong(lock, newLock, std::memory_order_acquire, std::memory_order_relaxed))
			return newLock;
	}
}

bool MutexPrivate::lock(TimePoint timeout)
{
	for (int i = 0; i < m_spinCount; ++i)
	{
		unsigned int expected = g_freeSpinValue;
		const unsigned int cpu = cpuCurrentId();
		if (m_lockCpu.compare_exchange_strong(expected, cpu, std::memory_order_acquire, std::memory_order_relaxed))
			return true;

		if ((expected >= g_waitCountStartValue) || (expected == cpu))
			break;

		cpuPause();
	}

	if (EventObject::wait(timeout) == 0)
	{
		if (threadGetLocalData(LOCAL_THREAD_STORAGE_MUTEX_WAIT) == 1)
			onWake();

		return true;
	}
	
	return (onWake() < g_waitCountStartValue);
}

void MutexPrivate::unlock()
{
	unsigned int lock = m_lockCpu.load(std::memory_order_acquire);
	for (;;)
	{
		if (lock < g_waitCountStartValue)
		{
			if (m_lockCpu.compare_exchange_strong(lock, g_freeSpinValue, std::memory_order_acquire, std::memory_order_relaxed))
				break;
		}
		else
		{
			EventObject::set();
			break;
		}
	}
}

bool MutexPrivate::try_lock()
{
	unsigned int expected = g_freeSpinValue;
	return m_lockCpu.compare_exchange_strong(expected, cpuCurrentId(), std::memory_order_acquire, std::memory_order_relaxed);
}

bool MutexPrivate::onWaitBegin()
{
	unsigned int lock = m_lockCpu.load(std::memory_order_acquire);
	for (;;)
	{
		if (lock == g_freeSpinValue)
		{
			if (m_lockCpu.compare_exchange_strong(lock, cpuCurrentId(), std::memory_order_acquire, std::memory_order_relaxed))
			{
				threadSetLocalData(LOCAL_THREAD_STORAGE_MUTEX_WAIT, 0);
				return false;
			}
		}
		else
		{
			const unsigned int newLock = ((lock < g_waitCountStartValue) ? g_waitCountStartValue : (lock + 1));
			if (m_lockCpu.compare_exchange_strong(lock, newLock, std::memory_order_acquire, std::memory_order_relaxed))
				break;
		}
	}
	threadSetLocalData(LOCAL_THREAD_STORAGE_MUTEX_WAIT, 1);
	return true;
}

bool MutexPrivate::onAddTaskToWaitList(Task*)
{
	return onWaitBegin();
}

bool MutexPrivate::addTaskToWaitList(Task* task)
{
	return EventObject::addTaskToWaitList(task);
}

kmutex::kmutex(int spinCount)
	: m_private(new (&m_privateMemory) MutexPrivate(spinCount))
{
	static_assert(sizeof(kmutex::m_privateMemory) >= sizeof(MutexPrivate));
}

kmutex::~kmutex()
{
	m_private->~MutexPrivate();
}

bool kmutex::lock(TimePoint timeout)
{
	return m_private->lock(timeout);
}

void kmutex::unlock()
{
	m_private->unlock();
}

bool kmutex::try_lock()
{
	return m_private->try_lock();
}