/*
   Process.cpp
   Kernel process manager
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

#include <algorithm>
#include <VirtualMemoryManager.h>
#include "panic.h"
#include "Process.h"

Process::Process()
	: m_supervisor(true)
	, m_vmm(VirtualMemoryManager::system())
{
}

Process::~Process()
{
	PANIC(L"not implemented");
}

void Process::addThread(kthread* thread)
{
	klock_guard lock(m_threadsMutex);
	m_threads.push_back(thread);
}

void Process::removeThread(kthread* thread)
{
	klock_guard lock(m_threadsMutex);
	m_threads.remove_first(thread);
}

bool Process::addModule(AbstractModule* module)
{
	{
		klock_guard lock(m_modulesMutex);
		if (std::find(m_modules.begin(), m_modules.end(), module) != m_modules.end())
			return false;

		m_modules.push_back(module);
	}
	module->load();
	return true;
}

bool Process::removeModule(AbstractModule* module)
{
	module->unload();
	{
		klock_guard lock(m_modulesMutex);
		if (!m_modules.remove_first(module))
			return false;
	}
	return true;
}

AbstractModule* Process::moduleByName(const kstring& name) const
{
	klock_guard lock(m_modulesMutex);
	for (AbstractModule* module : m_modules)
	{
		if (module->name() == name)
			return module;
	}
	return nullptr;
}

Process& Process::kernel()
{
	static Process process;
	return process;
}
