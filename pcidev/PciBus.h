/*
   PciBus.h
   PCI driver header
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

#pragma once
#include <memory>
#include <atomic>
#include <kunordered_map.h>
#include <klist.h>
#include <PciDevice.h>
#include <AbstractDevice.h>
#include <IoResource.h>
#include <pci.h>
#include <kshared_mutex.h>


struct PciDeviceState
{
    PciDeviceInfo m_info;
    std::unique_ptr<IoResource> m_pciConf;
    std::unique_ptr<PciDevice> m_device;
    std::unique_ptr<kmutex> m_initDevMutex;
};


class PciBusEnumerator: public AbstractDevice
{
public:
    static PciBusEnumerator& instance();

    void printDeviceList();
    bool addClassCodeFactory(unsigned int classCode, unsigned int subclassCode, const kvector<unsigned int>& progInterface, PciDeviceDriver* driver);
    void removeClassCodeFactory(unsigned int classCode, unsigned int subclassCode);
    void onDeleteDevice(PciDeviceState* pciState);

private:
    PciBusEnumerator();
    PciBusEnumerator(const PciBusEnumerator&) = delete;
    PciBusEnumerator(PciBusEnumerator&&) = delete;
    PciBusEnumerator& operator=(const PciBusEnumerator&) = delete;
    PciBusEnumerator& operator=(PciBusEnumerator&&) = delete;

    void search();
    void processDevice(const PciAddress& pciAddr, std::unique_ptr<IoResource>&& pciConf, uint32_t devVenId);
    void checkForClassCode(unsigned int classCode, unsigned int subclassCode, const kvector<unsigned int>& progInterface, PciDeviceDriver* driver);

private:
    struct ClassCodeFactoryEntry
    {
        kvector<unsigned int> progInterface;
        PciDeviceDriver* m_driver;
    };
    

private:
    kmutex m_factoryMutex;
    kshared_mutex m_devMutex;
    kunordered_map<unsigned int, klist<PciDeviceState>> m_devs;
    kunordered_map<unsigned int, ClassCodeFactoryEntry> m_ccFactory;
};
