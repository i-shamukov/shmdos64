/*
   main.cpp
   PCI generic IRQ routing
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

#include <pci.h>
#include <memory>


struct PciBusRounting
{
    PciBusRounting(unsigned int parent, unsigned int device)
        : m_parent(parent)
        , m_device(device)
    {
    }
    unsigned int m_parent;
    unsigned int m_device;
    unsigned int m_irq[PciLimits::Devices][PciLimits::Pins] = {};
};

static std::unique_ptr<PciBusRounting> g_routingTable[PciLimits::Buses]; 

PCI_SHARED unsigned int getPciDeviceIrq(unsigned int bus, unsigned int device, unsigned int pin)
{
    --pin;
    if ((pin >= PciLimits::Pins) || (bus >= PciLimits::Buses) || (device >= PciLimits::Devices))
        return 0;

    const std::unique_ptr<PciBusRounting>& routing = g_routingTable[bus];
    if (!routing)  
         return 0;
    
    unsigned int irq = routing->m_irq[device][pin]; 
    unsigned int offset = device + pin;
    while (irq == 0)
    {
        const unsigned int paretBus = g_routingTable[bus]->m_parent;
        if (bus == paretBus)
            break;

        offset += g_routingTable[bus]->m_device;
        bus = paretBus;
        irq = g_routingTable[bus]->m_irq[0][offset % PciLimits::Pins];
    }
    return irq;
}

PCI_SHARED bool registerPciRoutingBus(unsigned int bus, unsigned int parentBus, unsigned int device)
{
    if ((bus >= PciLimits::Buses) || (parentBus >= PciLimits::Buses) || (device >= PciLimits::Devices))
        return false;

    g_routingTable[bus] = std::make_unique<PciBusRounting>(parentBus, device);
    return true;
}

PCI_SHARED bool addPciDeviceRouting(unsigned int bus, unsigned int device, unsigned int pin, unsigned int irq)
{
    --pin;
    if ((pin >= PciLimits::Pins) || (bus >= PciLimits::Buses) || (device >= PciLimits::Devices))
        return false;
    
    std::unique_ptr<PciBusRounting>& routing = g_routingTable[bus];
    if (!routing)  
        return false;

    routing->m_irq[device][pin] = irq;
    return true;
}