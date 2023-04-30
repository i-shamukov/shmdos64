/*
   ExceptionHandlers.cpp
   Kernel default exceptions handlers
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
#include <kalgorithm.h>
#include <conout.h>
#include "ExceptionHandlers.h"
#include "paging.h"
#include "TaskManager.h"

static void defaultExceptionHandler(int type, InterruptVolatileState* state, bool ec)
{
	static const wchar_t* exceptionNames[] = {
		L"Divide-by-zero", //0
		L"Debug", //1
		L"Non-maskable Interrupt", //2
		L"Breakpoint", //3
		L"Overflow", //4
		L"Bound Range Exceeded", //5
		L"Invalid Opcode", //6
		L"Device Not Available", //7
		L"Double Fault", //8
		L"reserved 9", //9
		L"Invalid TSS", //10
		L"Segment Not Present", //11
		L"Stack-Segment Fault", //12
		L"General Protection Fault", //13
		L"Page Fault", //14
		L"reserved 15", //15
		L"x87 Floating-Point Exception", //16
		L"Alignment Check", //17
		L"Machine Check", //18
		L"SIMD Floating-Point Exception", //19
		L"Virtualization Exception", //20
	};
	const InterruptFrame& frame = ec ? state->m_frameEC.m_frame : state->m_frame;
	println(L"Unhandled exception: ", exceptionNames[type]);
	println(L"CPU_ID = ", cpuCurrentId());
	println(L"CS:RIP = ", hex(frame.m_cs, false), L':', hex(frame.m_rip, false));
	println(L"SS:RSP = ", hex(frame.m_ss, false), L':', hex(frame.m_rsp, false));
	println(L"EFLAGS = ", hex(frame.m_rflags, false));
	println(L"CR2 = ", hex(cpuGetCR2(), false));
	const uintptr_t stackAddr = reinterpret_cast<uintptr_t>(state) + sizeof(InterruptVolatileState);
	const uint8_t* stackBytes = reinterpret_cast<const uint8_t*>(stackAddr);
	print(L"STACK: ");
	size_t printBytes = kmin((PAGE_SIZE - (stackAddr & PAGE_MASK)), 256ULL);
	while (printBytes-- > 0)
		print(hex(*stackBytes++));
	print(L'\n');
	if (ec)
		println(L"ERROR_CODE = ", hex(state->m_frameEC.m_errorCode));

	cpuStop();
}

EXCEPTION_HANDLER(SystemDivideErrorExeption)
{
	defaultExceptionHandler(0, state, false);
}

EXCEPTION_HANDLER(SystemDebugExeption)
{
	defaultExceptionHandler(1, state, false);
}

EXCEPTION_HANDLER(SystemNmiExeption)
{
	(void)state;
}

EXCEPTION_HANDLER(SystemBreakpointExeption)
{
	defaultExceptionHandler(3, state, false);
}

EXCEPTION_HANDLER(SystemOverflowExeption)
{
	defaultExceptionHandler(4, state, false);
}

EXCEPTION_HANDLER(SystemBoundExeption)
{
	defaultExceptionHandler(5, state, false);
}

EXCEPTION_HANDLER(SystemInvalidOpcodeExeption)
{
	defaultExceptionHandler(6, state, false);
}

EXCEPTION_HANDLER(SystemNoMathExeption)
{
	if (!TaskManager::onUseFpu())
		defaultExceptionHandler(7, state, false);
}

EXCEPTION_HANDLER_EC(SystemDoubleFaultExeption)
{
	defaultExceptionHandler(8, state, true);
}

EXCEPTION_HANDLER_EC(SystemInvalidTSSExeption)
{
	defaultExceptionHandler(10, state, true);
}

EXCEPTION_HANDLER_EC(SystemSegmentExeption)
{
	defaultExceptionHandler(11, state, true);
}

EXCEPTION_HANDLER_EC(SystemStackSegmentExeption)
{
	defaultExceptionHandler(12, state, true);
}

EXCEPTION_HANDLER_EC(SystemGeneralProtectionExeption)
{
	defaultExceptionHandler(13, state, true);
}

EXCEPTION_HANDLER_EC(PageFaultHandler)
{
	if (!PagingManager64::current()->onPageFault(cpuGetCR2(), state->m_frameEC.m_errorCode))
		defaultExceptionHandler(14, state, true);
}

EXCEPTION_HANDLER(SystemFPUExeption)
{
	defaultExceptionHandler(16, state, false);
}

EXCEPTION_HANDLER_EC(SystemAlignmentExeption)
{
	defaultExceptionHandler(17, state, true);
}

EXCEPTION_HANDLER(SystemHardwareExeption)
{
	defaultExceptionHandler(18, state, false);
}

EXCEPTION_HANDLER(SystemSSEExeption)
{
	defaultExceptionHandler(19, state, false);
}

EXCEPTION_HANDLER(SystemVirtualizationExeption)
{
	defaultExceptionHandler(20, state, false);
}

namespace ExceptionHandlers
{
	void install()
	{
		SystemIDT::setHandler(0, &SystemDivideErrorExeption, true);
		SystemIDT::setHandler(1, &SystemDebugExeption, true);
		SystemIDT::setHandler(2, &SystemNmiExeption, true);
		SystemIDT::setHandler(3, &SystemBreakpointExeption, true);
		SystemIDT::setHandler(4, &SystemOverflowExeption, true);
		SystemIDT::setHandler(5, &SystemBoundExeption, true);
		SystemIDT::setHandler(6, &SystemInvalidOpcodeExeption, true);
		SystemIDT::setHandler(7, &SystemNoMathExeption, true);
		SystemIDT::setHandler(8, &SystemDoubleFaultExeption, true);
		SystemIDT::setHandler(10, &SystemInvalidTSSExeption, true);
		SystemIDT::setHandler(11, &SystemSegmentExeption, true);
		SystemIDT::setHandler(12, &SystemStackSegmentExeption, true);
		SystemIDT::setHandler(13, &SystemGeneralProtectionExeption, true);
		SystemIDT::setHandler(14, &PageFaultHandler, true);
		SystemIDT::setHandler(16, &SystemFPUExeption, true);
		SystemIDT::setHandler(17, &SystemAlignmentExeption, true);
		SystemIDT::setHandler(18, &SystemHardwareExeption, true);
		SystemIDT::setHandler(19, &SystemSSEExeption, true);
		SystemIDT::setHandler(20, &SystemVirtualizationExeption, true);
	}
}