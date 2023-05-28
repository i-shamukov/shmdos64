/*
   main.cpp
   Kernel main
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

#include <conout.h>
#include <cpu.h>
#include <VirtualMemoryManager.h>
#include "paging.h"
#include "gdt.h"
#include "bootlib.h"
#include "smp.h"
#include "idt.h"
#include "ExceptionHandlers.h"
#include "phmem.h"
#include "tests.h"
#include "LocalApic.h"
#include "AcpiTables.h"
#include "ExternalInterrupts.h"
#include "Hpet.h"
#include "TaskManager.h"
#include "InterruptQueuePool.h"
#include "PeLoader.h"
#include "panic.h"
#include "KernelPower.h"

void KernelMain()
{
	println(L"Startup SHM DOS64");
	PagingManager64::unmapBootPages();
	SystemSMP::initBootCpu();
	SystemGDT::installOnBootCpu();
	SystemIDT::install();
	ExceptionHandlers::install();
	VirtualMemoryManager::system();
	LocalApic::system().initCurrentCpu();
	AcpiTables::instance();
	ExternalInterrupts::system();
	Hpet::install();
	TaskManager::init();
	SystemSMP::init();
	InterruptQueuePool::system();
	KernelPower::init();
	PeLoader::loadKernelModules();
	runTests();
	
	TaskManager::terminateCurrentTask();
	PANIC(L"Failed to terminate thread");
}