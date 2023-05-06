/*
   kspin_lock.h
   Shared header for SHM DOS64
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
#include <kernel_export.h>

template <bool> class QueuedSpinLockBase;
class KERNEL_SHARED kspin_lock
{
public:
    kspin_lock();
    ~kspin_lock();
    void lock();
	void unlock();
	bool try_lock();

private:
	kspin_lock(const kspin_lock&) = delete;
	kspin_lock(kspin_lock&&) = delete;
	kspin_lock& operator=(const kspin_lock&) = delete;

private:
    QueuedSpinLockBase<true>* m_private;
};
