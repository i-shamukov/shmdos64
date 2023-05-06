/*
   EventObject.cpp
   Kernel event object
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
#include "panic.h"
#include "EventObject.h"
#include "TaskManager.h"
#include "AbstractTimer.h"

kevent::kevent()
	: m_private(new EventObject())
{

}

kevent::kevent(bool set, bool manualReset)
	: m_private(new EventObject(set, manualReset))
{

}

void kevent::set()
{
	return m_private->set();
}

uint32_t kevent::waitExStatus(TimePoint timeout)
{
	return m_private->waitExStatus(timeout);
}

bool kevent::wait(TimePoint timeout)
{
	return (m_private->waitExStatus(timeout) < WaitTimeout);
}

uint32_t kevent::waitMultiple(kevent** objects, uint32_t numObjects, bool waitAll, TimePoint timeout)
{
	if ((numObjects == 0) || (numObjects > TaskWaitEventsMax))
		return WaitError;

	EventObject* privObjects[TaskWaitEventsMax];
	for (uint32_t idx = 0; idx < numObjects; ++idx)
		privObjects[idx] = objects[idx]->m_private;

	return EventObject::waitMultiple(privObjects, numObjects, waitAll, timeout);
}

kevent::~kevent()
{
	delete m_private;
}

EventObject::EventObject()
{

}

EventObject::EventObject(bool set, bool manualReset)
	: m_set(set)
	, m_manualReset(manualReset)
{

}

EventObject::~EventObject()
{
	if (m_waitTaskListHead != nullptr)
	{
		PANIC(L"Attempting to destroy event that threads are waiting");
	}
}

void EventObject::removeTask(Task* task)
{
	Task::EventsInfo* info = taskEventInfo(task);
	if (info->m_next != nullptr)
	{
		taskEventInfo(info->m_next)->m_prev = info->m_prev;
		if (info->m_prev != nullptr)
			taskEventInfo(info->m_prev)->m_next = info->m_next;
		else
			m_waitTaskListHead = info->m_next;
	}
	else
	{
		m_waitTaskListTail = info->m_prev;
		if (m_waitTaskListTail != nullptr)
			taskEventInfo(m_waitTaskListTail)->m_next = nullptr;
		else
			m_waitTaskListHead = nullptr;
	}
}

void EventObject::insertTask(Task* task)
{
	Task::EventsInfo& info = task->m_waitEventsInfo[task->m_waitEventCount++];
	info.m_next = nullptr;
	info.m_prev = m_waitTaskListTail;
	info.m_object = this;
	if (m_waitTaskListTail != nullptr)
		taskEventInfo(m_waitTaskListTail)->m_next = task;
	else
		m_waitTaskListHead = task;
	m_waitTaskListTail = task;
}

Task::EventsInfo* EventObject::taskEventInfo(Task* task) const
{
	for (size_t idx = 0; idx < task->m_waitEventCount; ++idx)
	{
		Task::EventsInfo& info = task->m_waitEventsInfo[idx];
		if (info.m_object == this)
			return &info;
	}
	PANIC(L"Event not found in task");
	return nullptr;
}

void EventObject::excludeEventFromTask(Task::EventsInfo& info)
{
	if (info.m_next != nullptr)
	{
		info.m_object->taskEventInfo(info.m_next)->m_prev = info.m_prev;
		if (info.m_prev != nullptr)
			info.m_object->taskEventInfo(info.m_prev)->m_next = info.m_next;
		else
			info.m_object->m_waitTaskListHead = info.m_next;
	}
	else
	{
		if (info.m_prev != nullptr)
			info.m_object->taskEventInfo(info.m_prev)->m_next = nullptr;
		else
			info.m_object->m_waitTaskListHead = nullptr;
		info.m_object->m_waitTaskListTail = info.m_prev;
	}
}

Task::EventsInfo* EventObject::excludeEventsFromTask(Task* task) const
{
	Task::EventsInfo* result = nullptr;
	for (size_t idx = 0; idx < task->m_waitEventCount; ++idx)
	{
		Task::EventsInfo& info = task->m_waitEventsInfo[idx];
		if (info.m_object == this)
			result = &info;
		else if (info.m_object != nullptr)
		{
			klock_guard lock(info.m_object->m_spin);
			excludeEventFromTask(info);
		}
	}

	if (result == nullptr)
	{
		PANIC(L"Event not found in task");
	}
	return result;
}

void EventObject::set()
{
	klock_guard lock(m_spin);
	if (m_set)
		return;

	m_set = true;
	while (m_waitTaskListHead != nullptr)
	{
		Task* task = m_waitTaskListHead;
		Task::EventsInfo* info;
		{
			klock_guard lock(task->m_spin);

			if ((task->m_state != Task::State::Sleep) && (task->m_state != Task::State::TimedSleep))
			{
				m_waitTaskListHead = taskEventInfo(task)->m_next;
				continue;
			}

			if (task->m_needWaitEvents > 1)
			{
				info = taskEventInfo(task);
				--task->m_needWaitEvents;
				info->m_object = nullptr;
			}
			else
			{
				if (!m_manualReset && !onWakeSingleThread(task))
				{
					m_set = false;
					break;
				}

				info = excludeEventsFromTask(task);
				task->m_waitEventCount = 0;
				task->m_state = Task::State::Wait;
				task->m_waitEventResult = info - &task->m_waitEventsInfo[0];
				TaskManager::system()->addWaitTask(task);
			}
		}
		if (!m_manualReset)
		{
			m_set = false;
			m_waitTaskListHead = info->m_next;
			break;
		}
		m_waitTaskListHead = info->m_next;
	}

	if (m_waitTaskListHead != nullptr)
		taskEventInfo(m_waitTaskListHead)->m_prev = nullptr;
	else
		m_waitTaskListTail = nullptr;
}

uint32_t EventObject::waitExStatus(TimePoint timeout)
{
	Task* task = TaskManager::current();
	{
		klock_guard lock(m_spin);
		if (m_set)
		{
			if (!m_manualReset)
				m_set = false;
			return 0;
		}

		if (!onWaitBegin())
			return 0;

		if (timeout == 0)
			return WaitTimeout;

		if (!TaskManager::prepareToSleep(task, timeout))
			return WaitError;

		task->m_waitEventResult = WaitError;
		insertTask(task);
	}
	return task->m_waitEventResult;
}

bool EventObject::wait(TimePoint timeout)
{
	return (waitExStatus(timeout) < WaitTimeout);
}

uint32_t EventObject::waitMultiple(EventObject** objects, uint32_t numObjects, bool waitAll, TimePoint timeout)
{	
	bool needWait[TaskWaitEventsMax];
	Task* task = TaskManager::current();
	{
		uint32_t setCount = 0;
		TaskSwitchLock swLock;
		for (size_t idx = 0; idx < numObjects; ++idx)
		{
			EventObject* obj = objects[idx];
			obj->m_spin.lock();
			if (obj->m_set || !obj->onWaitBegin())
			{
				if (!waitAll)
				{
					if (obj->m_set && !obj->m_manualReset)
						obj->m_set = false;
					for (size_t unlockIdx = 0; unlockIdx < idx; ++unlockIdx)
						obj->m_spin.unlock();
					return idx;
				}
				else
				{
					++setCount;
				}
				obj->m_spin.unlock();
				needWait[idx] = false;
			}
			else
			{
				needWait[idx] = true;
			}
		}
		
		auto unlockAll = [numObjects, task, &objects, &needWait] (bool insert){
			for (size_t idx = 0; idx < numObjects; ++idx)
			{
				if (needWait[idx])
				{
					EventObject* obj = objects[idx];
					if (insert)
						obj->insertTask(task);
					obj->m_spin.unlock();
				}
			}
		};
		
		if (setCount == numObjects)
		{
			unlockAll(false);
			return 0;
		}
		
		if (timeout == 0)
		{
			unlockAll(false);
			return WaitTimeout;
		}
			
		if (!TaskManager::prepareToSleep(task, timeout))
		{
			unlockAll(false);
			return WaitError;
		}

		unlockAll(true);
		task->m_needWaitEvents = waitAll ? numObjects : 1;
	}

	return task->m_waitEventResult;
}

void EventObject::excludeTaskFromWaiting(Task* task, uint32_t reason)
{
	task->m_waitEventResult = reason;
	for (size_t idx = 0; idx < task->m_waitEventCount; ++idx)
	{
		Task::EventsInfo& info = task->m_waitEventsInfo[idx];
		if (info.m_object != nullptr)
		{
			klock_guard lock(info.m_object->m_spin);
			excludeEventFromTask(info);
		}
	}
	task->m_waitEventCount = 0;
}

void EventObject::excludeFromTask(Task* task)
{
	for (size_t idx = 0; idx < task->m_waitEventCount; ++idx)
	{
		Task::EventsInfo& info = task->m_waitEventsInfo[idx];
		if (info.m_object == this)
		{
			excludeEventFromTask(info);
			info.m_object = nullptr;
			break;
		}
	}
	--task->m_needWaitEvents;
}

// task spin locked
bool EventObject::addTaskToWaitList(Task* task)
{
	klock_guard lock(m_spin);
	if (!onAddTaskToWaitList(task))
		return false;

	insertTask(task);
	++task->m_needWaitEvents;
	return true;
}
