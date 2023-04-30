/*
   TaskManager.cpp
   Kernel task manager
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
#include "smp.h"
#include "ThreadLocalStorage.h"
#include "panic.h"
#include "ThreadPrivate.h"
#include "paging.h"
#include "gdt.h"
#include "idt.h"
#include "TaskManager.h"
#include "LocalApic.h"

#include <conout.h>

#define ROUTINE_INT_TO_STR_HELPER(value) #value
#define ROUTINE_INT_TO_STR(value) ROUTINE_INT_TO_STR_HELPER(value)
#define EXTERN_BEGIN_ROUTINE\
	"call cpuFastEio\n"\
    "movq FS:[" ROUTINE_INT_TO_STR(LOCAL_CPU_NEED_TASK_SWITCH_MACRO) "], 1\n"\
    "cmpq FS:[" ROUTINE_INT_TO_STR(LOCAL_CPU_MT_LOCK_COUNT_MACRO) "], 0\n"\
    "jz 3f\n"\
    "iretq\n"\
    "3:\n"\
    "movq FS:[" ROUTINE_INT_TO_STR(LOCAL_CPU_MT_LOCK_COUNT_MACRO) "], 1\n"\
    "sti\n"

#define TASK_SWITCH_HANDLER(procName, beginRoutine)\
    asm volatile(   ".globl " #procName "\n"\
                    #procName ":\n"\
					beginRoutine\
                    INTERRUPT_SAVE_VOLATILE_REGS \
                    IRQ_HANDLER_END_ASM_ROUTINE\
                    INTERRUPT_RESTORE_VOLATILE_REGS \
                    "iretq\n");\
    extern "C" void procName();\
	
void gccAsmFuncAddrBugWorkaround() { }
TASK_SWITCH_HANDLER(localTaskSwitchHandler, "");
TASK_SWITCH_HANDLER(externTaskSwitchHandler, EXTERN_BEGIN_ROUTINE);

static TaskManager* g_systemTaskManager = nullptr;

static void updateNextSheduleTime(TimePoint timepoint)
{
	cpuSetLocalData(LOCAL_CPU_SHED_TIME, timepoint);
}

static TimePoint getNextSheduleTime()
{
	return cpuGetLocalData(LOCAL_CPU_SHED_TIME);
}

static Task* idleTask()
{
	return static_cast<Task*>(cpuGetLocalPtr(LOCAL_CPU_IDLE_TASK));
}

static bool checkNeedTaskSwitch()
{
	CpuInterruptLock lock;
	const uintptr_t lockCount = cpuGetLocalData(LOCAL_CPU_MT_LOCK_COUNT);
	if ((lockCount > 1) || (cpuGetLocalData(LOCAL_CPU_NEED_TASK_SWITCH) == 0))
	{
		cpuSetLocalData(LOCAL_CPU_MT_LOCK_COUNT, lockCount - 1);
		return false;
	}

	return true;
}

static void cpuIdleThreadProc()
{
	for (;;)
	{
		cpuHalt();
	}
}

TaskManager::TaskManager()
	: m_numCpu(cpuLogicalCount())
	, m_apic(LocalApic::system())
	, m_forcedTaskSwitchTimeInterval(m_timer->fromMilliseconds(SYSTEM_FORCED_TASK_SWITCH_TIME_MS))
	, m_idleInfo(m_numCpu)
{
	m_kernelMainThread.m_private = new ThreadPrivate(&m_kernelMainThread, Process::kernel(), std::function<void()>(), false, 0, 0);
	SystemIDT::setHandler(CPU_LOCAL_TASK_SW_VECTOR, &localTaskSwitchHandler, false);
	SystemIDT::setHandler(CPU_EXTERN_TASK_SW_VECTOR, &externTaskSwitchHandler, true);
	for (IdleInfo& idleInfo : m_idleInfo)
	{
		kthread* idleThread = new kthread(&cpuIdleThreadProc, false);
		Task* task = idleThread->m_private->m_task.get();
		task->m_idle = true;
		idleInfo.m_task = task;
		idleInfo.m_run = false;
	}
}

TaskManager::~TaskManager()
{
	PANIC(L"not implemented");
	for (IdleInfo& idleInfo : m_idleInfo)
		delete idleInfo.m_task->m_threadPrivate->m_obj;
}

TaskManager* TaskManager::system()
{
	return g_systemTaskManager;
}

void TaskManager::init()
{
	TaskManager* mgr = new TaskManager();
	mgr->initCurrentCpu(&mgr->m_kernelMainThread);
	mgr->m_timer->updateTimePoint();
	g_systemTaskManager = mgr;
	mgr->m_timer->setEnableInterrupt(true);
}

void TaskManager::initCurrentCpu(kthread* firstThread)
{
	Task* task = firstThread->m_private->m_task.get();
	task->m_state = Task::State::Active;
	setCurrent(task);
	updateNextSheduleTime(AbstractTimer::system()->fastTimepoint());
	const unsigned int cpuId = cpuCurrentId();
	cpuSetLocalPtr(LOCAL_CPU_IDLE_TASK, m_idleInfo[cpuId].m_task);
	cpuSetLocalData(LOCAL_CPU_MT_LOCK_COUNT, 0);
}

void TaskManager::disableTaskSwitchingOnCurrentCPU()
{
	cpuLocalInc(LOCAL_CPU_MT_LOCK_COUNT);
}

void TaskManager::enableTaskSwitchingOnCurrentCPU()
{
	if (checkNeedTaskSwitch())
	{
		cpuCallInterrupt<CPU_LOCAL_TASK_SW_VECTOR>();
	}
}

Task* TaskManager::current()
{
	return static_cast<Task*>(threadGetLocalPtr(LOCAL_THREAD_STORAGE_CURRENT_TASK));
}

void TaskManager::setCurrent(Task* task)
{
	cpuWriteMSR(CPU_MSR_GS_BASE, reinterpret_cast<uintptr_t>(task->m_threadLocalData));
}

// currentTask locked
Task* TaskManager::shedule(Task* currentTask)
{
	if (m_realtimeListHead.load(std::memory_order_acquire) != nullptr)
	{
		kunique_lock lock(m_realtimeListSpin);
		for (;;)
		{
			Task* head = m_realtimeListHead.load(std::memory_order_relaxed);
			if (head == nullptr)
				break;

			m_realtimeListHead.store(head->m_realtimeNext, std::memory_order_release);
			lock.unlock();
			head->m_spin.lock();
			if (head->m_state == Task::State::PriorityWait)
				return head;

			head->m_spin.unlock();
			lock.lock();
		}
	}

	const TimePoint timepoint = m_timer->fastTimepoint();
	if (!currentTask->m_idle && (currentTask->m_state == Task::State::Active))
	{
		if (getNextSheduleTime() > timepoint)
			return currentTask;
	}

	class UpdateTimePointGuard
	{
	public:

		UpdateTimePointGuard(TimePoint timepoint)
			: m_timepoint(timepoint)
		{

		}

		~UpdateTimePointGuard()
		{
			updateNextSheduleTime(m_timepoint);
		};

	private:
		const TimePoint m_timepoint;
	} updateTimePointGuard(timepoint + m_forcedTaskSwitchTimeInterval);
	{
		kunique_lock lock(m_timedSleepTaskQueueSpin);
		while (!m_timedSleepTaskQueue.empty())
		{
			Task* newTask = m_timedSleepTaskQueue.minWakeTimeTask();
			if (newTask->m_wakeTime >= timepoint)
				break;

			m_timedSleepTaskQueue.pop();
			lock.unlock();
			newTask->m_spin.lock();
			newTask->m_priorityQueueIndex = Task::InvalidIndex;
			if (newTask->m_state == Task::State::TimedSleep)
			{
				EventObject::excludeTaskFromWaiting(newTask, EventObject::WaitTimeout);
				return newTask;
			}

			newTask->m_spin.unlock();
			lock.lock();
		}
	}
	{
		kunique_lock lock(m_activeTaskQueueSpin);
		if (!m_waitTaskQueue.empty())
		{
			do
			{
				Task* newTask = m_waitTaskQueue.minWakeTimeTask();
				m_waitTaskQueue.pop();
				
				lock.unlock();
				newTask->m_spin.lock();
				newTask->m_priorityQueueIndex = Task::InvalidIndex;
				if (newTask->m_state == Task::State::Wait)
					return newTask;

				newTask->m_spin.unlock();
				lock.lock();
			}
			while (!m_waitTaskQueue.empty());
		}
	}
	if (currentTask->m_state == Task::State::Active)
	{
		if (currentTask->m_idle)
			m_idleInfo[cpuCurrentId()].m_run.store(true, std::memory_order_release);
		return currentTask;
	}

	Task* idle = idleTask();
	idle->m_spin.lock();
	m_idleInfo[cpuCurrentId()].m_run.store(true, std::memory_order_release);
	return idle;
}

uintptr_t TaskManager::beginTaskSwitch(uintptr_t currentStack)
{
	Task* currentTask = current();
	if (!currentTask->m_readyForSleep)
		currentTask->m_spin.lock();
	else
		currentTask->m_readyForSleep = false;
	cpuSetLocalData(LOCAL_CPU_NEED_TASK_SWITCH, 0);
	Task* newTask = TaskManager::system()->shedule(currentTask);
	if (currentTask == newTask)
	{
		currentTask->m_spin.unlock();
		return 0;
	}

	cpuSetLocalPtr(LOCAL_CPU_NEXT_TASK, newTask);
	currentTask->m_stackTop = currentStack - offsetof(InterruptFullState, m_volatile);
	if (currentTask->m_useFpu)
		cpuSaveFpuContext(currentTask->m_fpuData);
	return newTask->m_stackTop;
}

void TaskManager::endTaskSwitch()
{
	Task* oldTask = current();
	Task* newTask = static_cast<Task*>(cpuGetLocalPtr(LOCAL_CPU_NEXT_TASK));
	const bool needSwitchPaging = (!newTask->m_kernel && (oldTask->m_pagingManager != newTask->m_pagingManager));
	TaskManager* mgr = TaskManager::system();
	mgr->m_contextSwitchCount++;
	bool oldTaskTerminated = false;
	if (oldTask->m_idle)
	{
		mgr->m_idleInfo[cpuCurrentId()].m_run.store(false, std::memory_order_release);
	}
	else if (oldTask->m_state == Task::State::Active)
	{
		klock_guard lock(mgr->m_activeTaskQueueSpin);
		oldTask->m_state = Task::State::Wait;
		oldTask->m_wakeTime = mgr->m_timer->fastTimepoint();
		if (newTask->m_state != Task::State::PriorityWait)
			oldTask->m_wakeTime += oldTask->m_desiredMaxWait;
		mgr->m_waitTaskQueue.push(oldTask);
	}
	else if (oldTask->m_state == Task::State::TimedSleep)
	{
		klock_guard lock(mgr->m_timedSleepTaskQueueSpin);
		mgr->m_timedSleepTaskQueue.push(oldTask);
	}
	else if (oldTask->m_state == Task::State::Terminated)
	{
		oldTaskTerminated = true;
	}
	oldTask->m_spin.unlock();
	if (oldTaskTerminated)
	{
		oldTask->m_threadPrivate->m_terminateEvent.set();
	}

	if (newTask->m_useFpu)
	{
		cpuRestoreFpuContext(newTask->m_fpuData);
		cpuClearTsFlag();
	}
	else
	{
		cpuSetTsFlag();
	}
	if (needSwitchPaging)
	{
		PagingManager64::setCurrent(newTask->m_pagingManager);
	}

	newTask->m_state = Task::State::Active;
	setCurrent(newTask);
	newTask->m_spin.unlock();
}

void TaskManager::onSystemTimerInterrupt()
{
	needTaskSwitch();
	system()->m_apic.sendBroadcastIpi(CPU_EXTERN_TASK_SW_VECTOR);
}

void TaskManager::needTaskSwitch()
{
	cpuSetLocalData(LOCAL_CPU_NEED_TASK_SWITCH, 1);
}

bool TaskManager::prepareToSleep(Task* task, TimePoint timeout)
{
	task->m_spin.lock();
	if (task->m_state == Task::State::Terminated)
	{
		task->m_spin.unlock();
		return false;
	}
	
	needTaskSwitch();
	task->m_readyForSleep = true;
	if (timeout != kevent::WaitInfinite)
	{
		task->m_state = Task::State::TimedSleep;
		task->m_wakeTime = AbstractTimer::system()->fastTimepoint() + timeout;
	}
	else
	{
		task->m_state = Task::State::Sleep;
	}
	task->m_waitEventResult = kevent::WaitError;
	return true;
}


void TaskManager::addWaitTask(Task* task)
{
	task->m_state = Task::State::Wait;
	task->m_wakeTime = m_timer->fastTimepoint() + task->m_desiredMaxWait;
	{
		klock_guard lock(m_activeTaskQueueSpin);
		m_waitTaskQueue.push(task);
	}
}

void TaskManager::addWaitTaskRealtime(Task* task)
{
	task->m_state = Task::State::PriorityWait;
	{
		klock_guard lock(m_realtimeListSpin);
		Task* head = m_realtimeListHead.load(std::memory_order_relaxed);
		task->m_realtimeNext = head;
		m_realtimeListHead.store(task, std::memory_order_relaxed);
	}
}

void TaskManager::smpBalancing()
{
	const unsigned int currCpuId = cpuCurrentId();
	auto checkCpu = [this](unsigned int cpuId) -> bool {
		IdleInfo& info = m_idleInfo[cpuId];
		bool desired = true;
		if (!info.m_run.load(std::memory_order_relaxed))
			return false;
		
		if (!info.m_run.compare_exchange_weak(desired, false, std::memory_order_acquire, std::memory_order_relaxed))
			return false;
		
		const TimePoint cutTime = m_timer->fastTimepoint();
		if (info.m_nextIpiTime > cutTime)
		{
			info.m_run.store(true, std::memory_order_release);
			return false;
		}
		
		info.m_nextIpiTime = cutTime + m_forcedTaskSwitchTimeInterval;
		return true;
	};
	if (!checkCpu(currCpuId))
	{
		for (unsigned int cpuId = 0; cpuId < m_numCpu; ++cpuId)
		{
			if ((cpuId != currCpuId) && checkCpu(cpuId))
			{
				m_apic.sendIpi(LocalApic::systemCpuIdToApic(cpuId), CPU_EXTERN_TASK_SW_VECTOR);
				return;
			}
		}
	}
	needTaskSwitch();
}

void TaskManager::terminateCurrentTask()
{
	TaskSwitchLock tsLock;
	Task* task = current();
	klock_guard taskLock(task->m_spin);
	if (task->m_state != Task::State::Active)
	{
		PANIC(L"current task is not Active");
	}

	task->m_state = Task::State::Terminated;
	needTaskSwitch();
}

bool TaskManager::onUseFpu()
{
	Task* task = current();
	klock_guard taskLock(task->m_spin);
	if (task->m_useFpu)
		return false;

	cpuClearTsFlag();
	cpuRestoreFpuContext(task->m_fpuData);
	task->m_useFpu = true;
	return true;
}

Task* TaskManager::extractTask(const kthread& thread)
{
	return thread.m_private->m_task.get();
}

extern "C"
{
	void tackManagerBeginInterrupt()
	{
		TaskManager::disableTaskSwitchingOnCurrentCPU();
	}

	uintptr_t tackManagerEndInterrupt(uintptr_t currentStack)
	{
		while (checkNeedTaskSwitch())
		{
			const uintptr_t newStack = TaskManager::beginTaskSwitch(currentStack);
			if (newStack != 0)
				return newStack;
		}
		return 0;
	}

	int tackManagerEndTaskSwitch()
	{
		TaskManager::endTaskSwitch();
		{
			CpuInterruptLock lock;
			if (cpuGetLocalData(LOCAL_CPU_NEED_TASK_SWITCH) == 0)
			{
				cpuSetLocalData(LOCAL_CPU_MT_LOCK_COUNT, 0);
				return 1;
			}
		}
		return 0;
	}
}

