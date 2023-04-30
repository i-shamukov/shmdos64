/*
   EventObject.h
   Kernel header
   SHM DOS64
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
#include <kvector.h>
#include <kevent.h>
#include "SpinLock.h"
#include "Task.h"

class AbstractTimer;
class EventObject
{
public:
	static const TimePoint WaitInfinite = kevent::WaitInfinite;
	static const uint32_t WaitError = kevent::WaitError;
	static const uint32_t WaitTimeout = kevent::WaitTimeout;

	EventObject();
	EventObject(bool set, bool manualReset);
	virtual ~EventObject();
	void set();
	uint32_t waitExStatus(TimePoint timeout);
	bool wait(TimePoint timeout = WaitInfinite);
	static uint32_t waitMultiple(EventObject** objects, uint32_t numObjects, bool waitAll, TimePoint timeout);

protected:
	bool addTaskToWaitList(Task* task);
	void excludeFromTask(Task* task);

private:
	EventObject(const EventObject&) = delete;
	EventObject(EventObject&&) = delete;
	EventObject& operator=(const EventObject&) = delete;
	void removeTask(Task* task);
	void insertTask(Task* task);
	Task::EventsInfo* taskEventInfo(Task* task) const;
	Task::EventsInfo* excludeEventsFromTask(Task* task) const;
	static void excludeEventFromTask(Task::EventsInfo& info);
	static void excludeTaskFromWaiting(Task* task, uint32_t reason);

	virtual bool onWaitBegin()
	{
		return true;
	}

	virtual bool onWakeSingleThread(Task*)
	{
		return true;
	}

	virtual bool onAddTaskToWaitList(Task*)
	{
		return true;
	}

private:
	bool m_set = false;
	bool m_manualReset = false;
	QueuedSpinLock m_spin;
	Task* m_waitTaskListTail = nullptr;
	Task* m_waitTaskListHead = nullptr;

	friend class TaskManager;
};
