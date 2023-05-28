/*
   pci.h
   Shared header for SHM DOS64
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
#include <common_types.h>
#include <kvector.h>


#ifdef PCI_EXPORT
   #define PCI_SHARED __declspec(dllexport)
#else
   #define PCI_SHARED
#endif

enum PciConfigurationSpaceHeader
{
   PciConfSpaceVendorId = 0x00,
   PciConfSpaceDeviceId = 0x02,
   PciConfSpaceVendorAndDeviceId = 0x00,
   PciConfSpaceCommand = 0x04,
   PciConfSpaceStatus = 0x06,
   PciConfSpaceCommandAndStatus = 0x04,
   PciConfSpaceFullClassId = 0x08,
   PciConfSpaceRevisionId = 0x08,
   PciConfSpaceProgInterface = 0x09,
   PciConfSubclass = 0x0A,
   PciConfClass = 0x0B,
   PciConfSpaceSubclassClass = 0x0A,
   PciConfComplexClassId = 0x08,
   PciConfCacheLineSize = 0x0C,
   PciConfLatencyTimer = 0x0D,
   PciConfHeaderType = 0x0E,
   PciConfBist = 0x0F
};

enum PciConfigurationHeaderType
{
   PciHeaderTypeDevice = 0x00,
   PciHeaderTypePciToPciBridge = 0x01
};

enum PciConfigurationSpacePciToPciBridge
{
   PciConfSecondaryBusNumber = 0x19
};

enum PciConfigurationSpaceDevice
{
   PciConfBar0 = 0x10,
   PciConfIrqPin = 0x3C,
   PciConfIrq = 0x3C,
   PciConfPin = 0x3D
};

enum PciClassCodes
{
   PciClassCodeBridge = 0x06
};

enum PciSubclassCodesBridge
{
   PciSubclassCodePciToPciBridge = 0x04
};

namespace PciLimits
{
   enum
   {
      Functions = 8,
      Devices = 32,
      Buses = 256,
      Pins = 4
   };
}

struct SystemEnhancedPciSegment
{
	uintptr_t m_mmioBase;
	unsigned int m_startBus;
	unsigned int m_endBus;
};

struct PciAddress
{
	unsigned int m_bus;
	unsigned int m_device;
	unsigned int m_function;
};

const kvector<SystemEnhancedPciSegment>& getSystemEnhancedPciSegments();

class IoResource;
IoResource* makePciSpaceIoResource(const PciAddress& pciAddr);

bool registerPciRoutingBus(unsigned int bus, unsigned int parentBus, unsigned int device);
bool addPciDeviceRouting(unsigned int bus, unsigned int device, unsigned int pin, unsigned int irq);
unsigned int getPciDeviceIrq(unsigned int bus, unsigned int device, unsigned int pin);
