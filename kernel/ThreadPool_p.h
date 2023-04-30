/*
   ThreadPool_p.h
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
#include <kthread.h>
#include <kvector.h>
#include <kmutex.h>
#include <kcondition_variable.h>
#include <klist.h>
#include <ThreadPool.h>

class ThreadPoolPrivate
{
public:
	ThreadPoolPrivate(size_t numThreads);
	~ThreadPoolPrivate();
	void run(const std::function<void()>& func);
	static ThreadPool& system();

private:
	ThreadPoolPrivate(const ThreadPool&) = delete;
	ThreadPoolPrivate(ThreadPool&&) = delete;
	ThreadPoolPrivate& operator=(const ThreadPool&) = delete;
	void threadProc();

private:
	kvector<kthread> m_threads;
	klist<std::function<void()>> m_queue;
	kcondition_variable m_cv;
	kmutex m_mutex;
	bool m_terminate = false;
};
