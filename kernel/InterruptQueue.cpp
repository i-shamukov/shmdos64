/*
   InterruptQueue.cpp
   Kernel interrupt-safe wait queue
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

#include <memory>
#include "SpinLock.h"
#include "TaskManager.h"
#include "InterruptQueuePrivate.h"
#include "EventObject.h"

class InterruptQueuePrivate
{
private:
	struct InterruptQueueNode
	{
		InterruptQueueNode* m_next;
		InterruptQueueNode* m_prev;
		InterruptQueue::Item m_item;
	};

public:
	InterruptQueuePrivate(size_t size)
		: m_nodes(std::make_unique<InterruptQueueNode[]>(size))
		, m_size(size)
	{
	}
	
	bool push(const InterruptQueue::Item& item)
	{
		Task* wakeTask = nullptr;
		{
			klock_guard lock(m_spin);
			InterruptQueueNode* newNode;
			if (m_free != nullptr)
			{
				newNode = m_free;
				m_free = m_free->m_next;
			}
			else
			{
				if (m_used == m_size)
					return false;

				newNode = &m_nodes[m_used];
				m_used++;
			}
			newNode->m_next = nullptr;
			if (m_tail != nullptr)
				m_tail->m_next = newNode;
			else
				m_head = newNode;
			newNode->m_prev = m_tail;
			newNode->m_item = item;
			m_tail = newNode;
			
			if (m_waitHead != nullptr)
			{
				wakeTask = m_waitHead;
				m_waitHead = m_waitHead->m_realtimeNext;
				if (m_waitHead == nullptr)
					m_waitTail = nullptr;
			}
		}
		if (wakeTask != nullptr)
		{
			wakeTask->m_waitEventResult = 0;
			TaskManager::system()->addWaitTaskRealtime(wakeTask);
		}
		return true;
	}
	
	bool pop(InterruptQueue::Item& item, TimePoint timeout)
	{
		for ( ; ; )
		{
			Task* task = nullptr;
			{
				klock_guard lock(m_spin);
				if (m_head != nullptr)
				{
					InterruptQueueNode* node = m_head;
					m_head = m_head->m_next;
					if (m_head != nullptr)
						m_head->m_prev = nullptr;
					else
						m_tail = nullptr;
					node->m_next = m_free;
					m_free = node;
					item = node->m_item;
					return true;
				}
				
				task = TaskManager::current();
				TaskManager::prepareToSleep(task, timeout);
				task->m_realtimeNext = nullptr;
				if (m_waitTail != nullptr)
					m_waitTail->m_realtimeNext = task;
				else
					m_waitHead = task;
				m_waitTail = task;
			}

			if (task->m_waitEventResult != 0)
				return false;
		}
	}

private:
	InterruptQueuePrivate(const InterruptQueuePrivate&) = delete;
	InterruptQueuePrivate(InterruptQueuePrivate&&) = delete;
	InterruptQueuePrivate& operator=(const InterruptQueuePrivate&) = delete;

private:
	std::unique_ptr<InterruptQueueNode[]> m_nodes;
	const size_t m_size;
	size_t m_used = 0;
	InterruptQueueNode* m_free = nullptr;
	InterruptQueueNode* m_head = nullptr;
	InterruptQueueNode* m_tail = nullptr;
	Task* m_waitHead = nullptr;
	Task* m_waitTail = nullptr;
	QueuedSpinLockIntLock m_spin;
};

InterruptQueue::InterruptQueue(size_t size)
	: m_private(new InterruptQueuePrivate(size))
{

}

InterruptQueue::~InterruptQueue()
{
	delete m_private;
}

bool InterruptQueue::push(const Item& item)
{
	return m_private->push(item);
}

bool InterruptQueue::pop(Item& item, TimePoint timeout)
{
	return m_private->pop(item, timeout);
}

