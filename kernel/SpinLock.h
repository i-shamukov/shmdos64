/*
   SpinLock.h
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
#include <atomic>
#include <klock_guard.h>

#define DEADLOCK_DEBUG

struct alignas(16) QueuedSpinLockEntry
{
	std::atomic<QueuedSpinLockEntry*> m_next;
	std::atomic<bool> m_state;
	std::atomic<bool> m_use = false;
};

template <bool StopMultitasking>
class QueuedSpinLockBase
{
public:
	void lock();
	void unlock();
	bool try_lock();

private:
	std::atomic<QueuedSpinLockEntry*> m_tail{ nullptr};
	QueuedSpinLockEntry* m_lastLockEntry = nullptr;
#ifdef DEADLOCK_DEBUG
	void *m_lockAdddr;
	unsigned int m_cpuId;
#endif
};

typedef QueuedSpinLockBase<true> QueuedSpinLock;
typedef QueuedSpinLockBase<false> QueuedSpinLockSm;

struct QueuedSpinLockIntLock
{
public:
	void lock();
	void unlock();

private:
	QueuedSpinLock m_lock;
	bool m_needInterrpuptLock;
};

void initSpinDataOnCurrentCpu();
