/*
   ExternalInterrupts.cpp
   Kernel IRQ & IOAPIC routines
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
#include <conout.h>
#include "idt.h"
#include "panic.h"
#include "AcpiTables.h"
#include "IoResource.h"
#include "TaskManager.h"
#include "ExternalInterrupts.h"

#define IRQ_HANDLER(procName)\
    asm volatile(   ".globl " #procName "\n"\
                    #procName ":\n"\
                    INTERRUPT_SAVE_VOLATILE_REGS \
                    IRQ_HANDLER_BEGIN_ASM_ROUTINE \
                    "call _"#procName "\n"\
                    IRQ_HANDLER_END_ASM_ROUTINE \
                    INTERRUPT_RESTORE_VOLATILE_REGS \
                    "iretq\n");\
    extern "C" void procName();\
    extern "C" void _##procName()

#define IRQ_HANDLER_DEF(irq) \
	IRQ_HANDLER(SystemHandlerIRQ##irq) { \
		allIrqHandler<irq>(); \
	}

enum
{
	IoApicVersionInternalReg = 0x01,
	IoApicRedirectionTableBaseInternalReg = 0x10
};

enum
{
	IoApicAddressReg = 0x00,
	IoApicValueReg = 0x10,
	IoApicMmioSize = 0x20
};

enum
{
	IoApicActivLow = (1 << 13),
	IoApicLevelSv = (1 << 15),
	IoApicMaskIrq = (1 << 16)
};

static inline unsigned int irqRegBase(unsigned int irq)
{
	return (2 * irq + IoApicRedirectionTableBaseInternalReg);
}

static inline void eoiRaw(unsigned)
{
	cpuFastEio();
}

template<unsigned int irq>
void allIrqHandler()
{
	ExternalInterrupts::InterruptData& intData = ExternalInterrupts::system().m_data[irq];
	++intData.m_irqActiveCounter;
	unsigned int eoiCnt = intData.m_eoiCounter;
	for (const ExternalInterrupts::IrqHandlerEntry& entry : intData.m_irqHandlers)
	{
		if (entry.m_proc(entry.m_obj))
			break;
	}
	--intData.m_irqActiveCounter;
	if (intData.m_eoiCounter == eoiCnt)
		eoiRaw(irq);
}

IRQ_HANDLER_DEF(0);
IRQ_HANDLER_DEF(1);
IRQ_HANDLER_DEF(2);
IRQ_HANDLER_DEF(3);
IRQ_HANDLER_DEF(4);
IRQ_HANDLER_DEF(5);
IRQ_HANDLER_DEF(6);
IRQ_HANDLER_DEF(7);
IRQ_HANDLER_DEF(8);
IRQ_HANDLER_DEF(9);
IRQ_HANDLER_DEF(10);
IRQ_HANDLER_DEF(11);
IRQ_HANDLER_DEF(12);
IRQ_HANDLER_DEF(13);
IRQ_HANDLER_DEF(14);
IRQ_HANDLER_DEF(15);
IRQ_HANDLER_DEF(16);
IRQ_HANDLER_DEF(17);
IRQ_HANDLER_DEF(18);
IRQ_HANDLER_DEF(19);
IRQ_HANDLER_DEF(20);
IRQ_HANDLER_DEF(21);
IRQ_HANDLER_DEF(22);
IRQ_HANDLER_DEF(23);
IRQ_HANDLER_DEF(24);
IRQ_HANDLER_DEF(25);
IRQ_HANDLER_DEF(26);
IRQ_HANDLER_DEF(27);
IRQ_HANDLER_DEF(28);
IRQ_HANDLER_DEF(29);
IRQ_HANDLER_DEF(30);
IRQ_HANDLER_DEF(31);

static void setupVectors()
{
	SystemIDT::setHandler(CPU_IRQ0_VECTOR + 0, &SystemHandlerIRQ0, true);
	SystemIDT::setHandler(CPU_IRQ0_VECTOR + 1, &SystemHandlerIRQ1, true);
	SystemIDT::setHandler(CPU_IRQ0_VECTOR + 2, &SystemHandlerIRQ2, true);
	SystemIDT::setHandler(CPU_IRQ0_VECTOR + 3, &SystemHandlerIRQ3, true);
	SystemIDT::setHandler(CPU_IRQ0_VECTOR + 4, &SystemHandlerIRQ4, true);
	SystemIDT::setHandler(CPU_IRQ0_VECTOR + 5, &SystemHandlerIRQ5, true);
	SystemIDT::setHandler(CPU_IRQ0_VECTOR + 6, &SystemHandlerIRQ6, true);
	SystemIDT::setHandler(CPU_IRQ0_VECTOR + 7, &SystemHandlerIRQ7, true);
	SystemIDT::setHandler(CPU_IRQ0_VECTOR + 8, &SystemHandlerIRQ8, true);
	SystemIDT::setHandler(CPU_IRQ0_VECTOR + 9, &SystemHandlerIRQ9, true);
	SystemIDT::setHandler(CPU_IRQ0_VECTOR + 10, &SystemHandlerIRQ10, true);
	SystemIDT::setHandler(CPU_IRQ0_VECTOR + 11, &SystemHandlerIRQ11, true);
	SystemIDT::setHandler(CPU_IRQ0_VECTOR + 12, &SystemHandlerIRQ12, true);
	SystemIDT::setHandler(CPU_IRQ0_VECTOR + 13, &SystemHandlerIRQ13, true);
	SystemIDT::setHandler(CPU_IRQ0_VECTOR + 14, &SystemHandlerIRQ14, true);
	SystemIDT::setHandler(CPU_IRQ0_VECTOR + 15, &SystemHandlerIRQ15, true);
	SystemIDT::setHandler(CPU_IRQ0_VECTOR + 16, &SystemHandlerIRQ16, true);
	SystemIDT::setHandler(CPU_IRQ0_VECTOR + 17, &SystemHandlerIRQ17, true);
	SystemIDT::setHandler(CPU_IRQ0_VECTOR + 18, &SystemHandlerIRQ18, true);
	SystemIDT::setHandler(CPU_IRQ0_VECTOR + 19, &SystemHandlerIRQ19, true);
	SystemIDT::setHandler(CPU_IRQ0_VECTOR + 20, &SystemHandlerIRQ20, true);
	SystemIDT::setHandler(CPU_IRQ0_VECTOR + 21, &SystemHandlerIRQ21, true);
	SystemIDT::setHandler(CPU_IRQ0_VECTOR + 22, &SystemHandlerIRQ22, true);
	SystemIDT::setHandler(CPU_IRQ0_VECTOR + 23, &SystemHandlerIRQ23, true);
	SystemIDT::setHandler(CPU_IRQ0_VECTOR + 24, &SystemHandlerIRQ24, true);
	SystemIDT::setHandler(CPU_IRQ0_VECTOR + 25, &SystemHandlerIRQ25, true);
	SystemIDT::setHandler(CPU_IRQ0_VECTOR + 26, &SystemHandlerIRQ26, true);
	SystemIDT::setHandler(CPU_IRQ0_VECTOR + 27, &SystemHandlerIRQ27, true);
	SystemIDT::setHandler(CPU_IRQ0_VECTOR + 28, &SystemHandlerIRQ28, true);
	SystemIDT::setHandler(CPU_IRQ0_VECTOR + 29, &SystemHandlerIRQ29, true);
	SystemIDT::setHandler(CPU_IRQ0_VECTOR + 30, &SystemHandlerIRQ30, true);
	SystemIDT::setHandler(CPU_IRQ0_VECTOR + 31, &SystemHandlerIRQ31, true);
}

static void disablePic()
{
	outportb(0x21, 0xFF);
	inportb(0x64);
	outportb(0xA1, 0xFF);
	inportb(0x64);
}

ExternalInterrupts::ExternalInterrupts()
	: AbstractDevice(DeviceClass::System, L"IO APIC", AbstractDevice::root())
{
	for (unsigned int irq = 0; irq < MaxIrq; irq++)
		m_remapTable[irq] = irq;
	setupVectors();
	initIoApic();
	cpuEnableInterrupts();
}

ExternalInterrupts::~ExternalInterrupts()
{
	PANIC(L"not implemented");
}

void ExternalInterrupts::initIoApic()
{
	const kvector<AcpiTables::IoApic>& ioApics = AcpiTables::instance().ioApics();
	if (ioApics.empty())
		PANIC(L"IO APIC table isn't present in ACPI");

	const AcpiTables::IoApic& ioApic = ioApics[0];
	if (ioApic.m_mmioBase == 0)
		PANIC(L"IO APIC MMIO is null");

	if (ioApic.m_irqBase != 0)
		PANIC(L"IO APIC irq base isn't null");

	m_mmio = io(ioApic.m_mmioBase, IoApicMmioSize, IoResourceType::MmioSpace);

	const uint32_t verReg = readData(IoApicVersionInternalReg);
	const size_t maxIrq = kmin<size_t>(((verReg >> 16) & 0xFF) + 1, MaxIrq);
	if (maxIrq < 24)
		PANIC(L"IOAPIC configuration unsupported (too few IRQ)");

	println(L"IOAPIC version ", hex(static_cast<uint8_t>(verReg & 0xFF), false));
	for (size_t irq = 0; irq < maxIrq; irq++)
	{
		m_data[irq].m_loCache = (CPU_IRQ0_VECTOR + irq) | IoApicMaskIrq;
		if (irq >= MaxIsaIrq)
			m_data[irq].m_loCache |= IoApicActivLow | IoApicLevelSv;
	}
	disablePic();
	for (const ApicIsaRemappingEntry& entry : AcpiTables::instance().isaRemapping())
	{
		if (entry.m_sourceIrq >= MaxIsaIrq)
			continue;

		InterruptData& intData = m_data[entry.m_apicIrq];
		uint32_t& apicReg = intData.m_loCache;
		if (entry.m_polarity == ApicIsaRemappingEntry::ActiveLow)
			apicReg |= IoApicActivLow;
		if (entry.m_triggerMode == ApicIsaRemappingEntry::LevelTriggered)
			apicReg |= IoApicLevelSv;
		if (entry.m_apicIrq != entry.m_sourceIrq)
		{
			InterruptData& othIntData = m_data[entry.m_sourceIrq];
			klock_guard lock1(intData.m_mutex);
			klock_guard lock2(othIntData.m_mutex);
			m_remapTable[entry.m_sourceIrq] = entry.m_apicIrq;
			intData.m_irqHandlers.swap(othIntData.m_irqHandlers);
		}
	}
}

void ExternalInterrupts::lockIrq(unsigned int irq)
{
	if (irq >= MaxIrq)
		return;

	irq = m_remapTable[irq];
	InterruptData& intData = m_data[irq];
	if (++intData.m_lockCounter == 1)
	{
		intData.m_loCache |= IoApicMaskIrq;
		writeData(irqRegBase(irq), intData.m_loCache);
	}
}

void ExternalInterrupts::unlockIrq(unsigned int irq)
{
	if (irq >= MaxIrq)
		return;

	irq = m_remapTable[irq];
	InterruptData& intData = m_data[irq];
	if (--intData.m_lockCounter == 0)
	{
		intData.m_loCache &= ~IoApicMaskIrq;
		writeData(irqRegBase(irq), intData.m_loCache);
	}
}

unsigned int ExternalInterrupts::installHandler(unsigned int irq, Handler proc, void* obj)
{
	static std::atomic<unsigned int> idCointer{0};
	if (irq >= MaxIrq)
		return 0;

	InterruptData& intData = m_data[m_remapTable[irq]];
	klock_guard lock(intData.m_mutex);
	if (!intData.m_irqHandlers.empty())
	{
		lockIrq(irq);
		while (intData.m_irqActiveCounter.load(std::memory_order_relaxed) != 0)
			cpuPause();
	}

	const unsigned int id = ++idCointer;
	intData.m_irqHandlers.emplace_back(proc, obj, id);
	unlockIrq(irq);
	return id;
}

bool ExternalInterrupts::deleteHandler(unsigned int irq, unsigned int id)
{
	if (irq >= MaxIrq)
		return false;

	bool result = false;
	InterruptData& intData = m_data[m_remapTable[irq]];
	klock_guard lock(intData.m_mutex);
	while (intData.m_irqActiveCounter.load(std::memory_order_relaxed) != 0)
		cpuPause();
	for (auto it = intData.m_irqHandlers.begin(); it != intData.m_irqHandlers.end();)
	{
		if (it->m_id == id)
		{
			it = intData.m_irqHandlers.erase(it);
			result = true;
			break;
		}
		else
		{
			++it;
		}
	}
	if (!intData.m_irqHandlers.empty())
		unlockIrq(irq);
	return result;
}

uint32_t ExternalInterrupts::readData(uint32_t reg) const
{
	m_mmio->out32(IoApicAddressReg, reg);
	return m_mmio->in32(IoApicValueReg);
}

void ExternalInterrupts::writeData(uint32_t reg, uint32_t value)
{
	m_mmio->out32(IoApicAddressReg, reg);
	m_mmio->out32(IoApicValueReg, value);
}

void ExternalInterrupts::eoi(unsigned int irq)
{
	if (irq >= MaxIrq)
		return;

	irq = m_remapTable[irq];
	eoiRaw(irq);
	++m_data[irq].m_eoiCounter;
}

ExternalInterrupts& ExternalInterrupts::system()
{
	static ExternalInterrupts exint;
	return exint;
}


