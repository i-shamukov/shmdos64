/*
   PciBus.cpp
   PCI bus enumerator
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

#include <kvector.h>
#include <conout.h>
#include "PciBus.h"


PciBusEnumerator::PciBusEnumerator()
    : AbstractDevice(DeviceClass::PciBus, L"PCI Bus", AbstractDevice::root(), AbstractDriver::kernel())
{
    search();
}

PciBusEnumerator& PciBusEnumerator::instance()
{
    static PciBusEnumerator pciBusEnumerator;
    return pciBusEnumerator;
}

void PciBusEnumerator::printDeviceList()
{
    println(L"PCI device list: ");
    println(L"BUS DEV FUNC VEN  DEV  CLASS IF BAR[0]   BAR[1]   BAR[2]   BAR[3]   BAR[4]   BAR[5]   PIN IRQ");
    for (const std::pair<unsigned int, klist<PciDeviceState>>& devList: m_devs)
    {
        for (const PciDeviceState& dev: devList.second)
        {
            print(hex(uint8_t(dev.m_info.m_pciAddress.m_bus), false), L"  ");
            print(hex(uint8_t(dev.m_info.m_pciAddress.m_device), false), L"  ");
            print(hex(uint8_t(dev.m_info.m_pciAddress.m_function), false), L"   ");
            print(hex(uint16_t(dev.m_info.m_vendorId), false), L' ');
            print(hex(uint16_t(dev.m_info.m_deviceId), false), L' ');
            print(hex(uint16_t((dev.m_info.m_classCode << 8) | dev.m_info.m_subclassCode), false), L"  ");
            print(hex(uint8_t(dev.m_info.m_progInterface), false), L' ');
            for (uint32_t bar : dev.m_info.m_bar)
                print(hex(bar, false), L' ');
            print(hex(uint8_t(dev.m_info.m_pin), false), L"  ");
            println(hex(uint8_t(dev.m_info.m_irq), false), L"  ");
        }
    }
}

void PciBusEnumerator::search()
{
    kvector<unsigned int> busStack = {0};
    bool scannedBus[PciLimits::Buses] = {true};
    while (!busStack.empty())
    {
        unsigned int curBus = busStack.back();
        busStack.pop_back();
        for (unsigned int dev = 0; dev < PciLimits::Devices; dev++)
        {
            for (unsigned int func = 0; func < PciLimits::Functions; func++)
            {
                const PciAddress pciAddr{curBus, dev, func};
                std::unique_ptr<IoResource> pciConf(makePciSpaceIoResource(pciAddr));
                if (!pciConf)
                    continue;
                
                const uint32_t devVenId = pciConf->in32(PciConfSpaceVendorAndDeviceId);
                if (devVenId == 0xFFFFFFFF)
                    continue;
                
                const uint8_t headerType = pciConf->in8(PciConfHeaderType);
                if ((headerType & 0x7F) == PciHeaderTypeDevice)
                {
                    processDevice(pciAddr, std::move(pciConf), devVenId);
                }
                else if ((headerType & 0x7F) == PciHeaderTypePciToPciBridge)
                {
                    const unsigned int bus = pciConf->in8(PciConfSecondaryBusNumber);
                    if (!scannedBus[bus])
                    {
                        scannedBus[bus] = true;
                        busStack.push_back(bus);
                    }
                }
                if ((func == 0) && ((headerType & 0x80) == 0))
                    break;
            }
        }
    }
}

void PciBusEnumerator::processDevice(const PciAddress& pciAddr, std::unique_ptr<IoResource>&& pciConf, uint32_t devVenId)
{
    PciDeviceState dev;
    dev.m_info.m_vendorId = devVenId & 0xFFFF;
    dev.m_info.m_deviceId = devVenId >> 16;
    const uint32_t complexClassId = pciConf->in32(PciConfComplexClassId);
    dev.m_info.m_progInterface = (complexClassId >> 8) & 0xFF;
    dev.m_info.m_subclassCode = (complexClassId >> 16) & 0xFF;
    dev.m_info.m_classCode = (complexClassId >> 24) & 0xFF;
    const uint16_t irqPin = pciConf->in16(PciConfIrqPin);
    dev.m_info.m_irq = irqPin & 0xFF;
    dev.m_info.m_pin = irqPin >> 8;
    if (dev.m_info.m_irq == 0xFF)
        dev.m_info.m_irq = 0;
    for (unsigned int barIdx = 0; barIdx < std::size(dev.m_info.m_bar); ++barIdx)
        dev.m_info.m_bar[barIdx] = pciConf->in32(PciConfBar0 + barIdx * 4);
    dev.m_info.m_pciAddress = pciAddr;
    dev.m_initDevMutex = std::make_unique<kmutex>();
    dev.m_pciConf = std::move(pciConf);
    {
        klock_guard devsLock(m_devMutex);
        klist<PciDeviceState>& devList = m_devs[(dev.m_info.m_classCode << 8) | dev.m_info.m_subclassCode];
        devList.emplace_back(std::move(dev));
    }
}

bool PciBusEnumerator::addClassCodeFactory(unsigned int classCode, unsigned int subclassCode, const kvector<unsigned int>& progInterface, PciDeviceDriver* driver)
{
    const unsigned int key = (classCode << 8) | subclassCode;
    {
        klock_guard lock(m_factoryMutex);
        if (m_ccFactory.find(key) != m_ccFactory.end())
            return false;

        m_ccFactory.emplace(key, ClassCodeFactoryEntry{progInterface, driver});
    }
    checkForClassCode(classCode, subclassCode, progInterface, driver);
    return true;
}

void PciBusEnumerator::removeClassCodeFactory(unsigned int classCode, unsigned int subclassCode)
{
    const unsigned int key = (classCode << 8) | subclassCode;
    klock_guard lock(m_factoryMutex);
    m_ccFactory.erase(key);
}

void PciBusEnumerator::checkForClassCode(unsigned int classCode, unsigned int subclassCode, const kvector<unsigned int>& progInterface, PciDeviceDriver* driver)
{
    const unsigned int key = (classCode << 8) | subclassCode;
    klock_shared devsLock(m_devMutex);
    auto it = m_devs.find(key);
    if (it == m_devs.end())
        return;

    for (PciDeviceState& devSt : it->second)
    {
        klock_guard lock(*devSt.m_initDevMutex);
        if (devSt.m_device)
            continue;
        
        if (!progInterface.empty()) 
        {
            if (std::find(progInterface.begin(), progInterface.end(), devSt.m_info.m_progInterface) == progInterface.end())
                continue;
        }

        PciDevice* dev = driver->deviceProbe(devSt.m_info, devSt.m_pciConf.get(), &devSt);
        if (dev == nullptr)
            continue;

        devSt.m_device.reset(dev);
    }
}

void PciBusEnumerator::onDeleteDevice(PciDeviceState* pciState)
{
    pciState->m_device.release();
}
