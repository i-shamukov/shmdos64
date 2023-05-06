/*
   ExternalInterrupts.h
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
#include <klist.h>
#include <kmutex.h>
#include <AbstractDevice.h>
#include <atomic>
#include "LocalApic.h"

class ExternalInterrupts : public AbstractDevice
{
public:
	static const size_t MaxIrq = 64;
	static const size_t MaxIsaIrq = 16;
	typedef bool (*Handler)(void* object);
	typedef void (*RawHandler)();

public:
	ExternalInterrupts();
	~ExternalInterrupts();
	void eoi(unsigned int irq);
	static ExternalInterrupts& system();
	void lockIrq(unsigned int irq);
	void unlockIrq(unsigned int irq);
	void lockIrqSafety(unsigned int irq);
	void unlockIrqSafety(unsigned int irq);
	unsigned int installHandler(unsigned int irq, Handler proc, void* obj);
	bool deleteHandler(unsigned int irq, unsigned int id);

private:
	ExternalInterrupts(const ExternalInterrupts&) = delete;
	ExternalInterrupts(ExternalInterrupts&&) = delete;
	void initIoApic();
	static uint32_t readData(IoResource* mmio, uint32_t reg);
	static void writeData(IoResource* mmio, uint32_t reg, uint32_t value);

private:
	struct IrqHandlerEntry
	{
		ExternalInterrupts::Handler m_proc;
		void* m_obj;
		unsigned int m_id;
		IrqHandlerEntry()
		{
		}
		IrqHandlerEntry(ExternalInterrupts::Handler proc, void* obj, int id)
			: m_proc(proc)
			, m_obj(obj)
			, m_id(id)
		{
		}
	};

private:
	struct InterruptData
	{
		IoResource* m_mmio = nullptr;
		uint32_t m_loCache;
		klist<IrqHandlerEntry> m_irqHandlers;
		kmutex m_mutex;
		unsigned int m_eoiCounter = 0;
		std::atomic<unsigned int> m_lockCounter{0};
		std::atomic<unsigned int> m_irqActiveCounter{0};
	} m_data[MaxIrq];
	unsigned int m_remapTable[MaxIrq];

	template<unsigned int irq> friend void allIrqHandler();
};
