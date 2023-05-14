/*
   PciIrqRouting.cpp
   ACPI PCI IRQ routing
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

extern "C"
{
#include <acpi.h>
}

#include <memory>
#include <algorithm>
#include <conout.h>
#include <pci.h>
#include <IoResource.h>
#include <kvector.h>
#include <common_lib.h>
#include "PciIrqRouting.h"


static bool setIrqApicMode()
{
    ACPI_OBJECT_LIST param;
    ACPI_OBJECT arg;
    arg.Type = ACPI_TYPE_INTEGER; 	                 
    arg.Integer.Value = 1;  // 1 = IOAPIC, 0 = PIC, 2 = IOSAPIC
    param.Count = 1;
    param.Pointer = &arg;
    static char picObject[] = "\\_PIC";
    return (AcpiEvaluateObject(NULL, picObject, &param, nullptr) == AE_OK);
}

static int getBridgePciBus(ACPI_HANDLE bridge, int parentPciBus)
{
    ACPI_BUFFER addrBuf;
    ACPI_OBJECT obj;
    
    addrBuf.Length = sizeof(obj);
    addrBuf.Pointer = &obj;
    static char bbnObject[] = "_BBN";
    if (AcpiEvaluateObject(bridge, bbnObject, NULL, &addrBuf) == AE_OK)
        return static_cast<int>(obj.Integer.Value);
    else
    {
        addrBuf.Length = sizeof(obj);
        addrBuf.Pointer = &obj;
        static char adrObject[] = "_ADR";
        AcpiEvaluateObject(bridge, adrObject, NULL, &addrBuf);
        const uint32_t pciDevAndFunc = obj.Integer.Value;
        if (obj.Type == ACPI_TYPE_INTEGER)
        {
            PciAddress pciAddr;
            pciAddr.m_bus = parentPciBus;
            pciAddr.m_device = (pciDevAndFunc >> 16) & 0xFFFF;
            pciAddr.m_function = pciDevAndFunc & 0xFFFF;
            std::unique_ptr<IoResource> pciSpace(makePciSpaceIoResource(pciAddr));
            if (pciSpace)
            {
                if ( (pciSpace->in8(PciConfHeaderType) & 0x7F) == PciHeaderTypePciToPciBridge)
                {
                    return pciSpace->in8(PciConfSecondaryBusNumber);
                }
            }
        }
    }
    return -1;
}

class PciIrqEnumerator
{
public:
    PciIrqEnumerator()
    {
        static char pciBridgeObject[] = "PNP0A03";
        AcpiGetDevices(pciBridgeObject, &PciIrqEnumerator::acpiProcessSystemBridge, this, nullptr);
        updatePciConf();
    }

private:
    struct CrsContext
    {
        PciIrqEnumerator* m_obj;
        ACPI_PCI_ROUTING_TABLE* m_routingTable;
        unsigned int m_bus;
    };

    static const int m_numPciIrqPins = 4;
    struct DeviceRouting
    {
        unsigned int m_bus;
        unsigned int m_device;
        unsigned int m_pinToIrq[m_numPciIrqPins] = {};
    };
    

private:
    void proceccBridge(ACPI_HANDLE bridge, int level, int parentPciBus)
    {
        if (level >= 5)
        {
            println(L"ACPI: too long PCI bus depth");
            return;
        }

        const int pciBus = getBridgePciBus(bridge, parentPciBus);
        if (pciBus == -1)
        {
            println(L"ACPI: failed to get pci bus");
            return;
        }

        ACPI_BUFFER rtblBuf;   
        rtblBuf.Length = ACPI_ALLOCATE_BUFFER;
        rtblBuf.Pointer = nullptr;
        if (AcpiGetIrqRoutingTable(bridge, &rtblBuf) == AE_OK)
        {
            uint8_t* tableBytes = static_cast<uint8_t*>(rtblBuf.Pointer);
            ACPI_PCI_ROUTING_TABLE* routingTable = static_cast<ACPI_PCI_ROUTING_TABLE*>(rtblBuf.Pointer);
            while (routingTable->Length != 0)
            {
                if (routingTable->Source[0] == '\0')
                    savePciIRQ(pciBus, (routingTable->Address >> 16) & 0x1F, routingTable->Pin, routingTable->SourceIndex);
                else
                {
                    ACPI_HANDLE srcHadle;
                    if (AcpiGetHandle(bridge, routingTable->Source, &srcHadle) != AE_OK)
                        continue;

                    CrsContext ctx{this, routingTable, static_cast<unsigned int>(pciBus)};
                    static char crsObject[] = "_CRS";
                    AcpiWalkResources(srcHadle, crsObject, &PciIrqEnumerator::processCrs, &ctx);
                }
                tableBytes += routingTable->Length;
                routingTable = reinterpret_cast<ACPI_PCI_ROUTING_TABLE*>(tableBytes);
            }
        }

    }

    static ACPI_STATUS processCrs(ACPI_RESOURCE* resource, void* context)
    {
        CrsContext* crsContext = (CrsContext*)context;
        ACPI_PCI_ROUTING_TABLE* routingTable = crsContext->m_routingTable;
        if (resource->Type == ACPI_RESOURCE_TYPE_IRQ) 
        {
            const ACPI_RESOURCE_IRQ* irq = &resource->Data.Irq;
            crsContext->m_obj->savePciIRQ(crsContext->m_bus, (routingTable->Address >> 16) & 0x1F, routingTable->Pin, irq->Interrupts[routingTable->SourceIndex]);    
        }
        else if (resource->Type == ACPI_RESOURCE_TYPE_EXTENDED_IRQ) 
        {
            const ACPI_RESOURCE_EXTENDED_IRQ* irq = &resource->Data.ExtendedIrq;
            crsContext->m_obj->savePciIRQ(crsContext->m_bus, (routingTable->Address >> 16) & 0x1F, routingTable->Pin, irq->Interrupts[routingTable->SourceIndex]);    
        }
        return AE_OK;
    }

    static ACPI_STATUS acpiProcessSystemBridge(ACPI_HANDLE bridge, UINT32, void* context, void**)
    {
        static_cast<PciIrqEnumerator*>(context)->proceccBridge(bridge, 0, 0);
        return AE_OK;
    }

    void savePciIRQ(unsigned int bus, unsigned int device, unsigned int pin, unsigned int irq)
    {
        const unsigned int maxIrq = 31;
        if ((irq >= maxIrq) || (pin >= m_numPciIrqPins))
            return;

        auto it = std::find_if(m_routing.begin(), m_routing.end(), [bus, device](const DeviceRouting& routing){
            return ((routing.m_bus == bus) && (routing.m_device == device));
        });
        if (it == m_routing.end())
        {
            DeviceRouting r;
            r.m_bus = bus;
            r.m_device = device;
            r.m_pinToIrq[pin] = irq;
            m_routing.emplace_back(std::move(r));
        }
        else
        {
            it->m_pinToIrq[pin] = irq;
        }
    }

    void updatePciConf()
    {
        print(L"PCI IRQ remapping: ");
        for (const DeviceRouting& routing : m_routing)
        {
            for (unsigned int function = 0; function < 8; function++)
            {
                PciAddress pciAddr;
                pciAddr.m_bus = routing.m_bus;
                pciAddr.m_device = routing.m_device;
                pciAddr.m_function = function;
                std::unique_ptr<IoResource> pciSpace(makePciSpaceIoResource(pciAddr));
                if (!pciSpace)
                    break;

                const uint16_t curIrqPin = pciSpace->in16(PciConfIrqPin);
                if (curIrqPin == 0xFFFF)
                    continue;

                const unsigned int pin = curIrqPin >> 8;
                const unsigned int oldIrq = curIrqPin & 0xFF;
                if (pin == 0)
                    continue;

                const unsigned int newIrq = routing.m_pinToIrq[pin - 1];
                if (chechRedirectIrq(oldIrq, newIrq))
                {
                    pciSpace->out8(PciConfIrq, newIrq);
                    print(L'[', routing.m_bus, L' ', routing.m_device, L' ', function, L"]=", newIrq, L' ');
                }
                if ((function == 0) && ((pciSpace->in16(PciConfHeaderType) & 0x80) == 0) ) 
                    break;
            }
        }
        println(L"");
    }

private:
    kvector<DeviceRouting> m_routing;
};

void initPciIrq()
{
    if (!setIrqApicMode())
    {
        println(L"ACPI: failed to setup IOAPIC mode");
        return;
    }
    PciIrqEnumerator();
}
