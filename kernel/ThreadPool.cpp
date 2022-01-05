/*
   ThreadPool.cpp
   Kernel thread pool
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
#include "ThreadPool_p.h"

ThreadPool::ThreadPool(size_t numThreads)
	: m_private(new ThreadPoolPrivate(numThreads))
{

}

ThreadPool::~ThreadPool()
{
	delete m_private;
}

void ThreadPool::run(const std::function<void()>& func)
{
	m_private->run(func);
}

ThreadPool& ThreadPool::system()
{
	static ThreadPool pool(cpuLogicalCount());
	return pool;
}

ThreadPoolPrivate::ThreadPoolPrivate(size_t numThreads)
{
	for (size_t idx = 0; idx < numThreads; ++idx)
		m_threads.emplace_back(std::bind(&ThreadPoolPrivate::threadProc, this));
}

ThreadPoolPrivate::~ThreadPoolPrivate()
{
	{
		klock_guard lock(m_mutex);
		m_terminate = true;
	}
	m_cv.notify_all();
	for (kthread& thread : m_threads)
		thread.join();
}

void ThreadPoolPrivate::run(const std::function<void()>& func)
{
	{
		klock_guard lock(m_mutex);
		m_queue.push_back(func);
	}
	m_cv.notify_one();
}

void ThreadPoolPrivate::threadProc()
{
	for (;;)
	{
		std::function<void()> func;
		{
			kunique_lock lock(m_mutex);
			while (m_queue.empty() && !m_terminate)
				m_cv.wait(lock);
			if (m_terminate)
				return;

			func = std::move(m_queue.front());
			m_queue.pop_front();
		}
		func();
	}
}
