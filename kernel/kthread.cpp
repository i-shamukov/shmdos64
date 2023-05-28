/*
   kthread.cpp
   Kernel thread object
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
#include "TaskManager.h"
#include "ThreadLocalStorage.h"
#include "idt.h"
#include "gdt.h"
#include "panic.h"
#include "ThreadPrivate.h"

static const TimePoint g_defaultDesiredTaskMaxWaitTimeMs = 10;

kthread::kthread()
	: m_private(nullptr)
{

}

kthread::kthread(const std::function<void()>& entry)
	: m_private(new ThreadPrivate(this, Process::kernel(), entry, true, DEFAULT_KERNEL_THREAD_STASK_SIZE, 0))
{

}

kthread::kthread(const std::function<void()>& entry, bool enqueue)
	: m_private(new ThreadPrivate(this, Process::kernel(), entry, enqueue, DEFAULT_KERNEL_THREAD_STASK_SIZE, 0))
{
	
}

kthread::kthread(Process& process, const std::function<void()>& entry, bool enqueue, size_t kernelStackSize, size_t userStackSize)
	: m_private(new ThreadPrivate(this, process, entry, enqueue, kernelStackSize, userStackSize))
{

}

kthread::~kthread()
{
	delete m_private;
}

kthread::kthread(kthread&& oth)
{
	m_private = oth.m_private;
	oth.m_private = nullptr;
}

kthread& kthread::operator=(kthread&& oth)
{
	if (this != &oth)
	{
		if (m_private != nullptr)
			delete m_private;

		m_private = oth.m_private;
		oth.m_private = nullptr;
	}
	return *this;
}

void kthread::join()
{
	m_private->m_terminateEvent.wait(EventObject::WaitInfinite);
}

namespace kthis_thread
{
	KERNEL_SHARED kthread::id get_id()
	{
		return reinterpret_cast<kthread::id>(TaskManager::current());
	}
}

void ThreadPrivate::init(size_t kernelStackSize)
{
	static const void* defaultFpuData = [] {
		void* result = VirtualMemoryManager::system().alloc(PAGE_SIZE, VMM_READWRITE);
		cpuSaveFpuContext(result);
		return result;
	}();
	if (kernelStackSize != 0)
	{
		m_task->m_systemStack = m_sysVmm.alloc(kernelStackSize, (m_process.supervisor() ? (VMM_READWRITE | VMM_COMMIT) : VMM_READWRITE));
	}
	else
	{
		m_task->m_systemStack = nullptr;
	}
	m_task->m_stackTop = reinterpret_cast<uintptr_t>(m_task->m_systemStack) + kernelStackSize;
	m_task->m_fpuData = m_sysVmm.alloc(PAGE_SIZE, VMM_READWRITE);
	m_task->m_threadLocalData = m_sysVmm.alloc(LOCAL_THREAD_STORAGE_DATA_SIZE, VMM_READWRITE);
	kmemcpy(m_task->m_fpuData, defaultFpuData, PAGE_SIZE);
	m_task->m_threadPrivate = this;
	uintptr_t* tls = static_cast<uintptr_t*>(m_task->m_threadLocalData);
	tls[LOCAL_THREAD_STORAGE_CURRENT_TASK / sizeof (*tls)] = reinterpret_cast<uintptr_t>(m_task.get());
	m_task->m_kernel = (&m_process == &Process::kernel());
	m_task->m_pagingManager = m_process.vmm().pagingManager();
	m_task->m_desiredMaxWait = AbstractTimer::system()->fromMilliseconds(g_defaultDesiredTaskMaxWaitTimeMs);
}

ThreadPrivate::ThreadPrivate(kthread* obj, Process& process, const std::function<void()>& entry, bool enqueue, size_t kernelStackSize, size_t /*userStackSize*/)
	: m_obj(obj)
	, m_process(process)
	, m_entry(entry)
{
	if (!process.supervisor())
	{
		PANIC(L"User mode not implemented");
	}

	m_process.addThread(m_obj);
	init(kernelStackSize);
	if (kernelStackSize != 0)
	{
		initStack(false);
		if (enqueue)
		{
			TaskSwitchLock lock;
			TaskManager* taskMgr = TaskManager::system();
			taskMgr->addWaitTask(m_task.get());
			taskMgr->smpBalancing();
		}
	}
}

ThreadPrivate::~ThreadPrivate()
{
	if (m_task->m_state != Task::State::Terminated)
	{
		PANIC(L"Attempt to destroy active thread");
	}

	m_sysVmm.free(m_task->m_systemStack);
	m_sysVmm.free(m_task->m_fpuData);
	m_sysVmm.free(m_task->m_threadLocalData);
	m_process.removeThread(m_obj);
}

void ThreadPrivate::initStack(bool user)
{
	InterruptFullState* is = reinterpret_cast<InterruptFullState*>(m_task->m_stackTop - sizeof(InterruptFullState));
	kmemset(is, 0, sizeof (*is));
	InterruptFrame& frame = is->m_volatile.m_frame;

	// subtraction sizeof(uintptr_t) for call emulation
	if (user)
	{
		frame.m_cs = USER_CODE_SEGMENT;
		frame.m_ss = USER_DATA_SEGMENT;
		frame.m_rsp = reinterpret_cast<uintptr_t>(m_userStack) - sizeof(uintptr_t);
	}
	else
	{
		frame.m_cs = SYSTEM_CODE_SEGMENT;
		frame.m_ss = SYSTEM_DATA_SEGMENT;
		frame.m_rsp = m_task->m_stackTop - 2 * sizeof(uintptr_t); // entryProc can use memory above RSP :( 
	}
	frame.m_rip = reinterpret_cast<uintptr_t>(&ThreadPrivate::entryProc);
	frame.m_rflags = 0x202;
	is->m_volatile.m_regs.m_rcx = reinterpret_cast<uintptr_t>(this);
	m_task->m_stackTop = reinterpret_cast<uintptr_t>(is);
}

void ThreadPrivate::entryProc(ThreadPrivate* objPriv)
{
	objPriv->m_entry();
	TaskManager::terminateCurrentTask();
}
