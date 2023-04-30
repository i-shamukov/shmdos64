/*
   Task.h
   Kernel task structure, routines and task priority queue
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
#include <kalgorithm.h>
#include <common_types.h>
#include <limits>
#include "SpinLock.h"

static const size_t TaskWaitEventsMax = 64;

class ThreadPrivate;
class PagingManager64;
class EventObject;

struct Task
{
	static constexpr size_t InvalidIndex = std::numeric_limits<size_t>::max();

	enum class State
	{
		None,
		Active,
		PriorityWait,
		Wait,
		Sleep,
		TimedSleep,
		Terminated
	};
	void* m_systemStack;
	uintptr_t m_stackTop;
	void* m_fpuData;
	void* m_threadLocalData;
	ThreadPrivate* m_threadPrivate;
	size_t m_priorityQueueIndex;
	Task* m_realtimeNext;

	struct EventsInfo
	{
		Task* m_next;
		Task* m_prev;
		EventObject* m_object;
	} m_waitEventsInfo[TaskWaitEventsMax];
	size_t m_waitEventCount = 0;
	uint32_t m_waitEventResult;
	uint32_t m_needWaitEvents = 0;
	State m_state = State::None;
	TimePoint m_wakeTime;
	QueuedSpinLockSm m_spin;
	bool m_kernel;
	bool m_useFpu = false;
	bool m_idle = false;
	TimePoint m_desiredMaxWait = 0;
	bool m_readyForSleep = false;
	PagingManager64* m_pagingManager;
};

static inline void setTaskTlsValue(Task* task, size_t offset, uintptr_t value)
{
	uint8_t* tlsData = static_cast<uint8_t*>(task->m_threadLocalData) + offset;
	kmemcpy(tlsData, &value, sizeof (value));
}

static inline void setTaskTlsPtr(Task* task, size_t offset, void* ptr)
{
	setTaskTlsValue(task, offset, reinterpret_cast<uintptr_t>(ptr));
}

static inline uintptr_t getTaskTlsValue(Task* task, size_t offset)
{
	uintptr_t value;
	const uint8_t* tlsData = static_cast<uint8_t*>(task->m_threadLocalData) + offset;
	kmemcpy(&value, tlsData, sizeof (value));
	return value;
}

static inline void* getTaskTlsPtr(Task* task, size_t offset)
{
	return reinterpret_cast<void*> (getTaskTlsValue(task, offset));
}

class TaskPriorityQueue
{
public:
	TaskPriorityQueue(Task** pointersArray, size_t maxSize)
		: m_pointersArray(pointersArray)
		, m_maxSize(maxSize)
	{
	}

	bool push(Task* task)
	{
		if (m_curSize == m_maxSize)
			return false;

		size_t idx = m_curSize++;
		m_pointersArray[idx] = task;
		while (idx > 0)
		{
			const size_t newIdx = (idx - 1) / 2;
			if (compare(m_pointersArray[newIdx], m_pointersArray[idx]))
				break;

			kswap(m_pointersArray[newIdx], m_pointersArray[idx]);
			m_pointersArray[idx]->m_priorityQueueIndex = idx;
			idx = newIdx;
		}
		m_pointersArray[idx]->m_priorityQueueIndex = idx;
		return true;
	}

	bool empty() const
	{
		return (m_curSize == 0);
	}

	Task* minWakeTimeTask() const
	{
		return m_pointersArray[0];
	}

	void pop()
	{
		m_pointersArray[0] = m_pointersArray[--m_curSize];
		restore(0);
	}

	void remove(Task* task)
	{
		size_t idx = task->m_priorityQueueIndex;
		Task * const lastTask = m_pointersArray[--m_curSize];
		m_pointersArray[idx] = lastTask;
		lastTask->m_priorityQueueIndex = idx;
		restore(idx);
	}

private:
	TaskPriorityQueue(const TaskPriorityQueue&) = delete;
	TaskPriorityQueue(TaskPriorityQueue&&) = delete;
	TaskPriorityQueue& operator=(const TaskPriorityQueue&) = delete;

private:
	bool compare(const Task* a, const Task* b) const
	{
		return (a->m_wakeTime < b->m_wakeTime);
	}

	void restore(size_t idx)
	{
		size_t left = 2 * idx + 1;
		size_t right = left + 1;
		size_t minIdx = 0;
		for (;;)
		{
			if ((left < m_curSize) && compare(m_pointersArray[left], m_pointersArray[minIdx]))
				minIdx = left;
			if ((right < m_curSize) && compare(m_pointersArray[right], m_pointersArray[minIdx]))
				minIdx = right;
			if (minIdx == idx)
				break;

			kswap(m_pointersArray[idx], m_pointersArray[minIdx]);
			m_pointersArray[idx]->m_priorityQueueIndex = idx;
			m_pointersArray[minIdx]->m_priorityQueueIndex = minIdx;
			idx = minIdx;
			left = 2 * idx + 1;
			right = left + 1;
		}
	}

private:
	Task** m_pointersArray;
	const size_t m_maxSize;
	size_t m_curSize = 0;
	QueuedSpinLockSm m_spin;
};
