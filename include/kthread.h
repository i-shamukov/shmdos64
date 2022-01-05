/*
   kthread.h
   Shared header for SHM DOS64
   Copyright (c) 2022, Ilya Shamukov <ilya.shamukov@gmail.com>
   
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
#include <functional>

enum
{
	DEFAULT_KERNEL_THREAD_STASK_SIZE = 0x10000,
	DEFAULT_USER_THREAD_STASK_SIZE = 0x100000
};

class Process;
class ThreadPrivate;
class kthread
{
public:
	kthread();
	kthread(kthread&& oth);
	kthread& operator=(kthread&&);
	kthread(const std::function<void()>& entry);
	kthread(const std::function<void()>& entry, bool enqueue);
	kthread(Process& process, const std::function<void()>& entry, bool enqueue = true, size_t kernelStackSize = DEFAULT_KERNEL_THREAD_STASK_SIZE, size_t userStackSize = 0);
	~kthread();
	void join();

private:
	kthread(const kthread&) = delete;
	kthread& operator=(const kthread&) = delete;

private:
	ThreadPrivate* m_private;

	friend class TaskManager;
};
