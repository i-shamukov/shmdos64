/*
   PciDevice.cpp
   PCI Device interface
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

#include <PciDevice.h>
#include "PciBus.h"

class PciDevicePrivate
{
private:
    PciAddress m_pciAddress;
};

class PciDeviceDriverPrivate
{
private:
    kvector<std::pair<unsigned int, unsigned int>> m_registeredClassCodes;
    
    friend class PciDeviceDriver;
};

PciDeviceDriver::PciDeviceDriver(const wchar_t* name)
    : AbstractDriver(name)
    , m_private(new PciDeviceDriverPrivate())
{

}

PciDeviceDriver::~PciDeviceDriver()
{
    PciBusEnumerator& pci = PciBusEnumerator::instance();
    for (const std::pair<unsigned int, unsigned int>& cc: m_private->m_registeredClassCodes)
        pci.removeClassCodeFactory(cc.first, cc.second);
    delete m_private;
}

bool PciDeviceDriver::addCompatibleClassCode(unsigned int classCode, unsigned int subclassCode, const kvector<unsigned int>& progInterface)
{
    if (!PciBusEnumerator::instance().addClassCodeFactory(classCode, subclassCode, progInterface, this))
        return false;

    m_private->m_registeredClassCodes.emplace_back(classCode, subclassCode);
    return true;
}

PciDevice* PciDeviceDriver::deviceProbe(const PciDeviceInfo&, IoResource*, PciDeviceState*)
{
    return nullptr;
}

PciDevice::PciDevice(DeviceClass deviceClass, const wchar_t* name, PciDeviceState* pciState, PciDeviceDriver* driver)
    : AbstractDevice(deviceClass, name, &PciBusEnumerator::instance(), driver)
    , m_pciState(pciState)
{

}

PciDevice::~PciDevice()
{
    static_cast<PciBusEnumerator*>(parent())->onDeleteDevice(m_pciState);
}


// TODO: MMIO Prefetchable
IoResource* PciDevice::io(unsigned int barIndex, size_t size)
{
    const uint32_t ioFlag = 0x01; 
    const uint32_t ioMask = 0xFFFC;
    const uint32_t mmioTypeMask = 0x06;
    const uint32_t mmioType64 = 0x06;
    const uint32_t mmioType32 = 0x00;
    const uint32_t mmioMask = 0xFFFFFFF0;
    const PciDeviceInfo& info = m_pciState->m_info;
    if (barIndex >= std::size(info.m_bar))
        return nullptr;

    const uint32_t lo = info.m_bar[barIndex];
    if ( (lo & ioFlag) != 0)
    {
        const uint16_t ioBase = (lo & ioMask);
        if (ioBase == 0)
            return nullptr;

        return AbstractDevice::io(ioBase, size, IoResourceType::IoSpace);
    }
    else
    {
        const uint32_t mmioType = lo & mmioTypeMask;
        if (mmioType == mmioType32)
        {
            const uint32_t mmioBase = (lo & mmioMask);
            return AbstractDevice::io(mmioBase, size, IoResourceType::MmioSpace);
        }
        else if ((mmioType == mmioType64) && (barIndex < (std::size(info.m_bar) - 1)))
        {
            const uint64_t hi = info.m_bar[barIndex + 1];
            const uint64_t mmioBase = (hi << 32) | (lo & mmioMask);
            return AbstractDevice::io(mmioBase, size, IoResourceType::MmioSpace);
        }
    }

    return nullptr;
}
