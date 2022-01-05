/*
   InterruptQueuePool.cpp
   Kernel iterrupt-safe wait queue pool
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

#include <cpu.h>
#include <AbstractDevice.h>
#include "InterruptQueuePool.h"

InterruptQueuePool::InterruptQueuePool(size_t size)
	: m_queue(size)
{
	const size_t numThreads = cpuLogicalCount();
	for (size_t idx = 0; idx < numThreads; ++idx)
		m_threads.emplace_back(std::bind(&InterruptQueuePool::threadProc, this));
}

InterruptQueuePool::~InterruptQueuePool()
{
	for (size_t idx = 0; idx < m_threads.size(); ++idx)
		m_queue.push(nullptr, 0, 0, nullptr);
	
	for (kthread& thread : m_threads)
		thread.join();
}

void InterruptQueuePool::postMessage(AbstractDevice* device, int arg1, int arg2, void* data)
{
	m_queue.push(device, arg1, arg2, data);
}

void InterruptQueuePool::threadProc()
{
	InterruptQueue::Item item;
	for ( ;  ;)
	{
		m_queue.pop(item, kevent::WaitInfinite);
		if (item.m_obj == nullptr)
			return;
		
		static_cast<AbstractDevice*>(item.m_obj)->onInterruptMessage(item.m_arg1, item.m_arg2, item.m_data);
	}
}

InterruptQueuePool& InterruptQueuePool::system()
{
	static InterruptQueuePool pool(0x100000);
	return pool;
}
