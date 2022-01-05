/*
   Process.h
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
#include <klist.h>
#include <kmutex.h>
#include "AbstractModule.h"

class VirtualMemoryManager;
class Process
{
public:
	~Process();
	bool supervisor() const
	{
		return m_supervisor;
	}
	VirtualMemoryManager& vmm()
	{
		return m_vmm;
	}
	bool addModule(AbstractModule* module);
	bool removeModule(AbstractModule* module);
	AbstractModule* moduleByName(const kstring& name) const;
	static Process& kernel();

private:
	Process();
	Process(const Process&) = delete;
	Process(Process&&) = delete;
	Process& operator=(const Process&) = delete;
	void addThread(kthread* thread);
	void removeThread(kthread* thread);

private:
	klist<kthread*> m_threads;
	kmutex m_threadsMutex;
	klist<AbstractModule*> m_modules;
	mutable kmutex m_modulesMutex;
	bool m_supervisor;
	VirtualMemoryManager& m_vmm;
	friend class ThreadPrivate;
};
