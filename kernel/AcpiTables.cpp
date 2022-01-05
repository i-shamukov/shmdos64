/*
   AcpiTables.cpp
   ACPI tables parser
   SHM DOS64
   Copyright (c) 2022, Ilya Shamukov, ilya.shamukov@gmail.com
   
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

#include <kernel_params.h>
#include <cpu.h>
#include <conout.h>
#include <vmem_utils.h>
#include <kernel_export.h>
#include "panic.h"
#include "AcpiTables.h"

#pragma pack(push, 1)

struct AcpiTableHeader
{
	uint8_t m_signature[4];
	uint32_t m_length;
	uint8_t m_revision;
	uint8_t m_checksum;
	uint8_t m_oemId[6];
	uint8_t m_oemTableID[8];
	uint8_t m_oemRevision[4];
	uint8_t m_creatorID[4];
	uint8_t m_creatorRevision[4];
};

struct Rsdp
{
	uint8_t m_signature[8];
	uint8_t m_checksum;
	uint8_t m_oemId[6];
	uint8_t m_revision;
	uint32_t m_rsdtAddress;
	uint32_t m_length;
	uint64_t m_xsdtAddress;
	uint8_t m_extendedChecksum;
	uint8_t m_reserved[3];
};

struct Xsdt
{
	AcpiTableHeader m_header;
	uint64_t m_entry[256];
};

struct Madt
{
	AcpiTableHeader m_header;
	uint32_t m_localAPICAddress;
	uint32_t m_flags;
	uint8_t m_apicTablesData[1024];
};

struct ApicTableHeader
{
	uint8_t m_type;
	uint8_t m_length;
};

struct LapicTable
{
	ApicTableHeader m_header;
	uint8_t m_acpiID;
	uint8_t m_apicID;
	uint32_t m_flags;
};

struct IoApicTable
{
	ApicTableHeader m_header;
	uint8_t m_ioApicID;
	uint8_t m_reserved;
	uint32_t m_address;
	uint32_t m_irqBase;
};

struct InterruptSourceTable
{
	ApicTableHeader m_header;
	uint8_t m_bus;
	uint8_t m_source;
	uint32_t m_globalSystemInterrupt;
	uint16_t m_flags;
};

struct McfgItem
{
	uint64_t m_mmioBase;
	uint16_t m_segment;
	uint8_t m_startBus;
	uint8_t m_endBus;
	uint8_t m_reserved[4];
};

struct Mcfg
{
	AcpiTableHeader m_header;
	uint8_t Reserved[8];
	McfgItem m_items[256];
};

struct AcpiGenericAddress
{
	uint8_t m_spaceId;
	uint8_t m_bitWidth;
	uint8_t m_bitOffset;
	uint8_t m_reserved;
	uint64_t m_mmmioBase;
};

struct HpetTable
{
	AcpiTableHeader m_header;
	uint32_t m_eventTimerBlockID;
	AcpiGenericAddress m_address;
	uint8_t m_number;
	uint16_t m_minimumClockTick;
	uint8_t m_attribute;
};

#pragma pack(pop) 

AcpiTables::AcpiTables()
{
	const Rsdp* rsdp = static_cast<const Rsdp*>(getKernelParams()->m_acpiRsdp);
	println(L"Found ACPI-compatible system");
	println(L"ACPI version: ", static_cast<int>(rsdp->m_revision + 1));
	println(L"RSDT address: ", hex(rsdp->m_xsdtAddress, false));
	print(L"OEMID: ");
	for (char oemChar : rsdp->m_oemId)
		print(oemChar);
	print(L'\n');

	const Xsdt* xsdt = physToVirtualInt<Xsdt>(rsdp->m_xsdtAddress);
	if (xsdt->m_header.m_length < (sizeof(*rsdp) - sizeof(rsdp->m_length)))
		PANIC(L"Incorrect size of ACPI RSDT");

	const size_t numTables = (xsdt->m_header.m_length - (sizeof(*xsdt) - sizeof(xsdt->m_entry))) / sizeof(*xsdt->m_entry);
	for (size_t tblIdx = 0; tblIdx < numTables; tblIdx++)
		m_tables.push_back(physToVirtualInt<AcpiTableHeader>(xsdt->m_entry[tblIdx]));

	parseApic();
	parseMcfg();
	parseHpet();
}

const void* AcpiTables::getTable(const TableSignature signature) const
{
	for (const AcpiTableHeader* tableHeader : m_tables)
	{
		if (kmemcmp(tableHeader->m_signature, signature, sizeof(tableHeader->m_signature)) == 0)
			return static_cast<const void*>(tableHeader);
	}

	return nullptr;
}

void AcpiTables::parseApic()
{

	enum
	{
		LapicType = 0,
		IoApicType = 1,
		InterruptSourceType = 2
	};
	const uint32_t lapicEnableFlag = 0x01;
	static const TableSignature madtSignature = {'A', 'P', 'I', 'C'};
	const Madt* madt = static_cast<const Madt*>(getTable(madtSignature));
	if (madt == nullptr)
		PANIC(L"MADT isn't found in ACPI");

	const uint8_t* apicDataPtr = madt->m_apicTablesData;
	const uint8_t* apicDataEndPtr = apicDataPtr + madt->m_header.m_length - (sizeof(*madt) - sizeof(madt->m_apicTablesData));
	while (apicDataPtr < apicDataEndPtr)
	{
		const ApicTableHeader* apicHeader = reinterpret_cast<const ApicTableHeader*>(apicDataPtr);
		if (apicHeader->m_length < sizeof (*apicHeader))
			PANIC(L"Empty MADT header");

		if ((apicHeader->m_type == LapicType) && (apicHeader->m_length == sizeof(LapicTable)))
		{
			const LapicTable* lapic = reinterpret_cast<const LapicTable*>(apicHeader);
			if ((lapic->m_flags & lapicEnableFlag) != 0)
			{
				println(L"Logical processor found: ACPI_ID = ", static_cast<int>(lapic->m_acpiID), L", LAPIC_ID = ", static_cast<int>(lapic->m_apicID));
				m_cpuApicIds.push_back(lapic->m_apicID);
			}
		}
		else if ((apicHeader->m_type == IoApicType) && (apicHeader->m_length == sizeof(IoApicTable)))
		{
			const IoApicTable* ioApic = reinterpret_cast<const IoApicTable*>(apicHeader);
			println(L"IOAPIC found, MMIO = ", hex(ioApic->m_address, false), L", IrqBase = ", ioApic->m_irqBase);
			m_ioApics.push_back(IoApic{ioApic->m_address, ioApic->m_irqBase});
		}
		else if ((apicHeader->m_type == InterruptSourceType) && (apicHeader->m_length == sizeof(InterruptSourceTable)))
		{
			const InterruptSourceTable* interruptSource = reinterpret_cast<const InterruptSourceTable*>(apicHeader);
			if (interruptSource->m_bus == 0)
			{
				println(L"APIC ISA IRQ remapping ", static_cast<int>(interruptSource->m_source), L"->", interruptSource->m_globalSystemInterrupt,
						L", Flags = ", hex(interruptSource->m_flags, false));
				ApicIsaRemappingEntry entry;
				entry.m_sourceIrq = interruptSource->m_source;
				entry.m_apicIrq = interruptSource->m_globalSystemInterrupt;
				entry.m_polarity = (interruptSource->m_flags & 0x03);
				entry.m_triggerMode = (interruptSource->m_flags >> 2) & 0x03;
				m_isaRemapping.push_back(entry);
			}
		}
		apicDataPtr += apicHeader->m_length;
	}
}

void AcpiTables::parseMcfg()
{
	static const TableSignature mcfgSignature = {'M', 'C', 'F', 'G'};
	const Mcfg* mcfg = static_cast<const Mcfg*>(getTable(mcfgSignature));
	if (mcfg == nullptr)
		return;

	const McfgItem* begin = &mcfg->m_items[0];
	const McfgItem* end = &mcfg->m_items[ (mcfg->m_header.m_length - (sizeof(*mcfg) - sizeof(mcfg->m_items))) / sizeof(*mcfg->m_items) ];
	for (const McfgItem* item = begin; item != end; item++)
	{
		println(L"PCI enhanced segment, START_BUS = ", static_cast<int>(item->m_startBus), L", END_BUS = ", static_cast<int>(item->m_endBus),
				L", MMIO = ", hex(item->m_mmioBase, false));
		if ((item->m_mmioBase == 0) || (item->m_startBus > item->m_endBus))
		{
			println(L"Invalid MCFG record");
			return;
		}

		SystemEnhancedPciSegment seg;
		seg.m_mmioBase = item->m_mmioBase;
		seg.m_startBus = item->m_startBus;
		seg.m_endBus = item->m_endBus;
		m_pciEnhancedSegments.push_back(seg);
	}
}

void AcpiTables::parseHpet()
{
	static const TableSignature hpetSign = {'H', 'P', 'E', 'T'};
	const HpetTable* table = static_cast<const HpetTable*>(getTable(hpetSign));
	if (table == nullptr)
		PANIC(L"HPET table isn't present in ACPI");

	if (table->m_address.m_spaceId != 0)
		PANIC(L"HPET registers space type unsupported");

	m_hpetMmioBase = table->m_address.m_mmmioBase;
	m_hpetMmioBase &= ~((1ULL << static_cast<uintptr_t>(table->m_address.m_bitOffset)) - 1);
	const uintptr_t hiBit = (1ULL << static_cast<uintptr_t>(table->m_address.m_bitWidth - 1));
	const uintptr_t mask = hiBit | (hiBit - 1);
	m_hpetMmioBase &= mask;
	if (m_hpetMmioBase == 0)
		PANIC(L"HPET MMIO base is null");

	println("HPET found, MMIO = ", hex(m_hpetMmioBase, false));
}

AcpiTables& AcpiTables::instance()
{
	static AcpiTables acpi;
	return acpi;
}

KERNEL_SHARED const kvector<SystemEnhancedPciSegment>& getSystemEnhancedPciSegments()
{
	return AcpiTables::instance().pciEnhancedSegments();
}
