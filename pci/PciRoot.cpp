/*
   PciRoot.cpp
   PCI bus driver implementaion
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
#include <AbstractDriver.h>
#include "PciRoot.h"
#include "PciLegacy.h"


PciRoot::PciRoot()
	: AbstractDevice(DeviceClass::PciBus, L"PCI bus", AbstractDevice::root(), AbstractDriver::kernel())
	, m_io(AbstractDevice::io(0x0CF8, 8, IoResourceType::IoSpace))
	, m_exSegments(getSystemEnhancedPciSegments())
{
	if (!PciLegacy::test(m_io))
		println(L"Error accessing to PCI\n");
}

PciRoot& PciRoot::instance()
{
	static PciRoot pci;
	return pci;
}

IoResource* PciRoot::makeIoResource(const PciAddress& pciAddr)
{
	const size_t g_enhancedPciSpaceSize = 0x1000;
	for (const SystemEnhancedPciSegment& exSeg : m_exSegments)
	{
		if ((pciAddr.m_bus >= exSeg.m_startBus) && (pciAddr.m_bus <= exSeg.m_endBus)) 
		{
			const uintptr_t offset = (( (pciAddr.m_bus & 0xFF) << 8) | ( (pciAddr.m_device & 0x1F) << 3) | (pciAddr.m_function & 0x07)) * g_enhancedPciSpaceSize;
			return makeMmioResource(exSeg.m_mmioBase + offset, g_enhancedPciSpaceSize);
		}
	}
	return new PciLegacy(m_io, pciAddr);
}

__declspec(dllexport) IoResource* makePciSpaceIoResource(const PciAddress& pciAddr)
{
	return PciRoot::instance().makeIoResource(pciAddr);
}
