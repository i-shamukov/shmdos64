/*
   SpinLock.cpp
   Kernel queued SpinLock
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
#include "SpinLock.h"
#include "smp.h"
#include "panic.h"

#include <conout.h>

#define TEMPLATE_FUNC_EXTERN(retType, funcName)\
    template retType QueuedSpinLockBase<true>::funcName();\
    template retType QueuedSpinLockBase<false>::funcName();

template<size_t size>
constexpr size_t align2nHigh()
{
	constexpr size_t next = size & -size;
	return next == size ? size : align2nHigh<size - next>();
}

struct LocalCpuSpinsData
{
	size_t m_count;
	static const size_t Limit = ((PAGE_SIZE - sizeof(m_count)) / sizeof(QueuedSpinLockEntry)) - 1;
	static const size_t LimitAlign = align2nHigh<Limit>();
	static const size_t LimitAlignMask = LimitAlign - 1;
	QueuedSpinLockEntry m_entry[LimitAlign];
};

static inline QueuedSpinLockEntry& allocSpinEntry()
{
	LocalCpuSpinsData* spinsData = static_cast<LocalCpuSpinsData*>(cpuGetLocalPtr(LOCAL_CPU_SPIN_DATA));
	QueuedSpinLockEntry* entryPtr;
	for (size_t idx = 0; idx < LocalCpuSpinsData::LimitAlign; ++idx)
	{
		entryPtr = &spinsData->m_entry[(++spinsData->m_count) & LocalCpuSpinsData::LimitAlignMask];
		bool expected = false;
		if (entryPtr->m_use.compare_exchange_weak(expected, true, std::memory_order_relaxed, std::memory_order_relaxed))
			return *entryPtr;
	}
	PANIC(L"Not enough spin entryes");
	return *entryPtr;
}

void initSpinDataOnCurrentCpu()
{
	LocalCpuSpinsData* spinsData = static_cast<LocalCpuSpinsData*>(cpuGetLocalPtr(LOCAL_CPU_SPIN_DATA));
	for (QueuedSpinLockEntry& entry : spinsData->m_entry)
		new (&entry) QueuedSpinLockEntry();
}

static inline void freeSpinEntry(QueuedSpinLockEntry& entry)
{
	entry.m_use.store(false, std::memory_order_relaxed);
}

template<bool StopMultitasking>
void QueuedSpinLockBase<StopMultitasking>::lock()
{
	if constexpr(StopMultitasking)
		TaskManager::disableTaskSwitchingOnCurrentCPU();
	QueuedSpinLockEntry& entry = allocSpinEntry();
	entry.m_next.store(nullptr, std::memory_order_relaxed);
	entry.m_state.store(true, std::memory_order_relaxed);
	QueuedSpinLockEntry* cur = m_tail.exchange(&entry, std::memory_order_acquire);
	if (cur != nullptr)
	{
		cur->m_next.store(&entry, std::memory_order_relaxed);
#ifdef DEADLOCK_DEBUG
		int cnt = 0;
#endif
		while (entry.m_state.load(std::memory_order_acquire))
		{
			cpuPause();
#ifdef DEADLOCK_DEBUG
			if (cnt++ == 10000)
			{
				static std::atomic<int> x{0};
				int val = 0;
				while (!x.compare_exchange_strong(val, 1, std::memory_order_acquire, std::memory_order_relaxed))
				{
					val = 0;
				}
				const void* ret = __builtin_return_address(0);
				const unsigned int cpuId = cpuCurrentId();
				//auto tm = AbstractTimer::system();
				println(/*tm->toMilliseconds(tm->timepoint()), ' ',*/ m_lockAdddr, ' ', m_cpuId, ' ', ret, ' ', cpuId);
				x = 0;
			}
#endif
		}
	}
	m_lastLockEntry = &entry;
#ifdef DEADLOCK_DEBUG
	m_lockAdddr = __builtin_return_address(0);
	if (TaskManager::system() != nullptr)
		m_cpuId = cpuCurrentId();
#endif
}

template<bool StopMultitasking>
void QueuedSpinLockBase<StopMultitasking>::unlock()
{
	QueuedSpinLockEntry& entry = *m_lastLockEntry;
	QueuedSpinLockEntry* expected = &entry;
	if (!m_tail.compare_exchange_strong(expected, nullptr, std::memory_order_acquire, std::memory_order_relaxed))
	{
		for (;;)
		{
			auto p = entry.m_next.load(std::memory_order_acquire);
			if (p != nullptr)
			{
				p->m_state.store(false, std::memory_order_release);
				break;
			}
		}
	}
	freeSpinEntry(entry);
	if constexpr(StopMultitasking)
		TaskManager::enableTaskSwitchingOnCurrentCPU();
}

template<bool StopMultitasking>
bool QueuedSpinLockBase<StopMultitasking>::try_lock()
{
	QueuedSpinLockEntry& entry = allocSpinEntry();
	entry.m_next.store(nullptr, std::memory_order_relaxed);
	entry.m_state.store(true, std::memory_order_relaxed);
	QueuedSpinLockEntry* expected = nullptr;
	if constexpr(StopMultitasking)
		TaskManager::disableTaskSwitchingOnCurrentCPU();
	if (!m_tail.compare_exchange_strong(expected, &entry, std::memory_order_acquire, std::memory_order_relaxed))
	{
		freeSpinEntry(entry);
		if constexpr(StopMultitasking)
			TaskManager::enableTaskSwitchingOnCurrentCPU();
		return false;
	}
	return true;
}

void QueuedSpinLockIntLock::lock()
{
	m_needInterrpuptLock = ((cpuGetFlagsRegister() & 0x200) != 0);
	if (m_needInterrpuptLock)
		cpuDisableInterrupts();
	m_lock.lock();
}

void QueuedSpinLockIntLock::unlock()
{
	if (m_needInterrpuptLock)
		cpuEnableInterrupts();
	m_lock.unlock();
}

TEMPLATE_FUNC_EXTERN(void, lock)
TEMPLATE_FUNC_EXTERN(void, unlock)
TEMPLATE_FUNC_EXTERN(bool, try_lock)