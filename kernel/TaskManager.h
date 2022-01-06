/*
   TaskManager.h
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
#include <kthread.h>
#include <kvector.h>
#include <memory>
#include "Task.h"
#include "AbstractTimer.h"

enum
{
	SYSTEM_FORCED_TASK_SWITCH_TIME_MS = 10,
	SYSTEM_TIMER_FREQUENCY_DIV = (1000 / SYSTEM_FORCED_TASK_SWITCH_TIME_MS)
};

extern "C"
{
	void tackManagerBeginInterrupt();
	uintptr_t tackManagerEndInterrupt(uintptr_t currentStack);
	int tackManagerEndTaskSwitch();
}

static size_t SystemMaxThreads = 0x10000;
class LocalApic;
class TaskManager
{
public:
	static void disableTaskSwitchingOnCurrentCPU();
	static void enableTaskSwitchingOnCurrentCPU();
	static Task* extractTask(const kthread& thread);
	void initCurrentCpu(kthread* firstThread);
	static Task* current();
	void addWaitTask(Task* task);
	void addWaitTaskRealtime(Task* task);
	void smpBalancing();

	uint64_t contextSwitchCount() const
	{
		return m_contextSwitchCount.load(std::memory_order_relaxed);
	}

	static TaskManager* system();
	static void init();
	static bool prepareToSleep(Task* task, TimePoint timeout);
	static void onSystemTimerInterrupt();
	static bool onUseFpu();

	static void terminateCurrentTask();

private:
	struct IdleInfo
	{
		Task* m_task = nullptr;
		std::atomic<bool> m_run{false};
		TimePoint m_nextIpiTime = 0;
	};

private:
	TaskManager();
	~TaskManager();
	TaskManager(const TaskManager&) = delete;
	TaskManager(TaskManager&&) = delete;
	static void setCurrent(Task* task);
	Task* shedule(Task* currentTask);
	static uintptr_t beginTaskSwitch(uintptr_t currentStack);
	static uintptr_t postInterrurptHandler(uintptr_t currentStack);
	static void endTaskSwitch();
	static void needTaskSwitch();

private:
	unsigned const int m_numCpu;
	LocalApic& m_apic;
	kthread m_kernelMainThread;
	std::atomic<Task*> m_realtimeListHead{nullptr};
	QueuedSpinLock m_realtimeListSpin;
	TaskPriorityQueue m_waitTaskQueue{new Task*[SystemMaxThreads], SystemMaxThreads};
	QueuedSpinLockSm m_activeTaskQueueSpin;
	TaskPriorityQueue m_timedSleepTaskQueue{new Task*[SystemMaxThreads], SystemMaxThreads};
	QueuedSpinLockSm m_timedSleepTaskQueueSpin;
	AbstractTimer* m_timer = AbstractTimer::system();
	const TimePoint m_forcedTaskSwitchTimeInterval;
	kvector<IdleInfo> m_idleInfo;
	std::atomic<uint64_t> m_contextSwitchCount;

	friend void tackManagerBeginInterrupt();
	friend uintptr_t tackManagerEndInterrupt(uintptr_t currentStack);
	friend int tackManagerEndTaskSwitch();
};

class TaskSwitchLock
{
public:
	TaskSwitchLock()
	{
		TaskManager::disableTaskSwitchingOnCurrentCPU();
	}
	~TaskSwitchLock()
	{
		TaskManager::enableTaskSwitchingOnCurrentCPU();
	}
};
