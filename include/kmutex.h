/*
   kmutex.h
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
#include <kernel_export.h>
#include <klock_guard.h>

class MutexPrivate;
class KERNEL_SHARED kmutex
{
public:
	kmutex(int spinCount = 512);
	~kmutex();
	void lock();
	void unlock();
	bool try_lock();

private:
	kmutex(const kmutex&) = delete;
	kmutex(kmutex&&) = delete;
	kmutex& operator=(const kmutex&) = delete;

private:
	MutexPrivate* m_private;
	char m_privateMemory[128];

	friend class ConditionVariablePrivate;
};