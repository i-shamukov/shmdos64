/*
   ThreadPrivate.h
   Kernel header
   SHM DOS64
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
#include <memory>
#include <VirtualMemoryManager.h>
#include <kthread.h>
#include "Process.h"
#include "EventObject.h"
#include <kthread.h>
#include "Task.h"

class ThreadPrivate
{
public:
	ThreadPrivate(kthread* obj, Process& process, const std::function<void()>& entry, bool enqueue, size_t kernelStackSize, size_t userStackSize);
	~ThreadPrivate();

private:
	ThreadPrivate(const ThreadPrivate&) = delete;
	ThreadPrivate(ThreadPrivate&&) = delete;
	ThreadPrivate& operator=(const ThreadPrivate&) = delete;
	void init(size_t systemStackSize);
	void initStack(bool user);

	static void entryProc(ThreadPrivate* objPriv);

private:
	VirtualMemoryManager& m_sysVmm = VirtualMemoryManager::system();
	std::unique_ptr<Task> m_task = std::make_unique<Task>();
	kthread* m_obj;
	Process& m_process;
	void* m_userStack = nullptr;
	EventObject m_terminateEvent{false, true};
	const std::function<void()> m_entry;

	friend class TaskManager;
	friend class kthread;
};
