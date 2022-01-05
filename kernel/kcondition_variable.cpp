/*
   kcondition_variable.cpp
   Kernel condition_variable
   SHM DOS64
   Copyright (c) 2022, Ilya Shamukov, ilya.shamukov@gmail.com
   
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

#include <kcondition_variable.h>
#include "ThreadLocalStorage.h"
#include "EventObject.h"
#include "kmutex_p.h"
#include "panic.h"

#include <conout.h>

class ConditionVariablePrivate : private EventObject
{
public:
	ConditionVariablePrivate()
	{

	}

	~ConditionVariablePrivate()
	{
		if (m_waitCount != 0)
			PANIC(L"object has waiting threads");
	}

	bool wait(kunique_lock<kmutex>& lock, TimeStamp timeout)
	{
		kmutex* mutex = lock.mutex();
		threadSetLocalPtr(LOCAL_THREAD_STORAGE_CV_MUTEX, mutex);
		threadSetLocalData(LOCAL_THREAD_STORAGE_CV_COUNT, m_notifyCount.load(std::memory_order_acquire));
		if (!EventObject::wait(timeout))
			return false;

		if (threadGetLocalData(LOCAL_THREAD_STORAGE_CV_MUTEX_LOCK) == 0)
		{
			kmutex* mutex = static_cast<kmutex*>(threadGetLocalPtr(LOCAL_THREAD_STORAGE_CV_MUTEX));
			mutex->m_private->onWake();
		}
		return true;
	}

	void notify_one()
	{
		++m_notifyCount;
		uint32_t waitCount = m_waitCount.load(std::memory_order_acquire);
		while (waitCount != 0)
		{
			if ((waitCount & m_testFlag) != 0)
			{
				waitCount = m_waitCount.load(std::memory_order_acquire);
				continue;
			}

			if (m_waitCount.compare_exchange_strong(waitCount, waitCount - 1, std::memory_order_acquire, std::memory_order_relaxed))
			{
				EventObject::set();
				break;
			}
		}

	}

	void notify_all()
	{
		while (m_waitCount.load(std::memory_order_acquire) != 0)
			notify_one();
	}

private:
	ConditionVariablePrivate(const kcondition_variable&) = delete;
	ConditionVariablePrivate(kcondition_variable&&) = delete;
	ConditionVariablePrivate& operator=(const kcondition_variable&) = delete;

	bool onWaitBegin() override
	{
		m_waitCount.fetch_or(m_testFlag, std::memory_order_acquire);
		if (m_notifyCount.load(std::memory_order_acquire) != threadGetLocalData(LOCAL_THREAD_STORAGE_CV_COUNT))
		{
			m_waitCount.fetch_and(~m_testFlag, std::memory_order_relaxed);
			threadSetLocalData(LOCAL_THREAD_STORAGE_CV_MUTEX_LOCK, 1);
			return false;
		}

		m_waitCount.fetch_add(m_testFlag + 1, std::memory_order_relaxed);
		kmutex* mutex = static_cast<kmutex*>(threadGetLocalPtr(LOCAL_THREAD_STORAGE_CV_MUTEX));
		mutex->unlock();
		return true;
	}

	bool onWakeSingleThread(Task* task) override
	{
		kmutex* mutex;
		const uint8_t* tlsData = static_cast<uint8_t*> (task->m_threadLocalData) + LOCAL_THREAD_STORAGE_CV_MUTEX;
		kmemcpy(&mutex, tlsData, sizeof (mutex));
		if (mutex->try_lock())
		{
			setTaskTlsValue(task, LOCAL_THREAD_STORAGE_CV_MUTEX_LOCK, 1);
			return true;
		}

		if (mutex->m_private->addTaskToWaitList(task))
		{
			setTaskTlsValue(task, LOCAL_THREAD_STORAGE_CV_MUTEX_LOCK, 0);
			EventObject::excludeFromTask(task);
			return false;
		}

		setTaskTlsValue(task, LOCAL_THREAD_STORAGE_CV_MUTEX_LOCK, 1);
		return true;
	}

private:
	std::atomic<uint32_t> m_waitCount{0};
	std::atomic<uint64_t> m_notifyCount{0};
	static const uint32_t m_testFlag = (1U << 31);
};

kcondition_variable::kcondition_variable()
	: m_private(new ConditionVariablePrivate())
{

}

kcondition_variable::~kcondition_variable()
{
	delete m_private;
}

bool kcondition_variable::wait(kunique_lock<kmutex>& lock, TimeStamp timeout)
{
	return m_private->wait(lock, timeout);
}

void kcondition_variable::notify_one()
{
	m_private->notify_one();
}

void kcondition_variable::notify_all()
{
	m_private->notify_all();
}
