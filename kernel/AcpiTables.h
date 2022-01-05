/*
   AcpiTables.h
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
#include <kvector.h>
#include <pci.h>
#include "LocalApic.h"

struct ApicIsaRemappingEntry
{
	enum
	{
		ActiveHigh = 0x01,
		ActiveLow = 0x03
	};

	enum
	{
		EdgeTriggered = 0x01,
		LevelTriggered = 0x03
	};

	unsigned int m_sourceIrq;
	unsigned int m_apicIrq;
	unsigned int m_polarity;
	unsigned int m_triggerMode;
};

struct AcpiTableHeader;
class AcpiTables
{
public:
	struct IoApic
	{
		uintptr_t m_mmioBase;
		unsigned int m_irqBase;
	};

public:
	AcpiTables();
	static AcpiTables& instance();

	const kvector<LocalApic::ApicCpuId>& cpuApicIds() const
	{
		return m_cpuApicIds;
	}

	const kvector<IoApic>& ioApics() const
	{
		return m_ioApics;
	}

	const kvector<ApicIsaRemappingEntry>& isaRemapping() const
	{
		return m_isaRemapping;
	}

	const kvector<SystemEnhancedPciSegment>& pciEnhancedSegments() const
	{
		return m_pciEnhancedSegments;
	}

	uintptr_t hpetMmioBase() const
	{
		return m_hpetMmioBase;
	}

private:
	typedef uint8_t TableSignature[4];
	AcpiTables(const AcpiTables& orig) = delete;
	AcpiTables(AcpiTables&&) = delete;
	const void* getTable(const TableSignature Signature) const;
	void parseApic();
	void parseMcfg();
	void parseHpet();

private:
	kvector<const AcpiTableHeader*> m_tables;
	kvector<LocalApic::ApicCpuId> m_cpuApicIds;
	kvector<IoApic> m_ioApics;
	kvector<ApicIsaRemappingEntry> m_isaRemapping;
	kvector<SystemEnhancedPciSegment> m_pciEnhancedSegments;
	uintptr_t m_hpetMmioBase = 0;
};
