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
    return (AcpiEvaluateObject(nullptr, picObject, &param, nullptr) == AE_OK);
}

static bool getPciDeviceAddr(ACPI_HANDLE device, PciAddress& pciAddr, int parentPciBus)
{
    ACPI_BUFFER addrBuf;
    ACPI_OBJECT obj;
    addrBuf.Length = sizeof(obj);
    addrBuf.Pointer = &obj;
    static char adrObject[] = "_ADR";
    if (AcpiEvaluateObject(device, adrObject, nullptr, &addrBuf) == AE_OK)
    {
        const uint32_t pciDevAndFunc = obj.Integer.Value;
        if (obj.Type == ACPI_TYPE_INTEGER)
        {
            pciAddr.m_bus = parentPciBus;
            pciAddr.m_device = (pciDevAndFunc >> 16) & 0xFFFF;
            pciAddr.m_function = pciDevAndFunc & 0xFFFF;
            return true;
        }
    }

    return false;
}

static int getBridgePciBus(ACPI_HANDLE bridge, const PciAddress& pciAddr)
{
    ACPI_BUFFER addrBuf;
    ACPI_OBJECT obj;
    addrBuf.Length = sizeof(obj);
    addrBuf.Pointer = &obj;
    static char bbnObject[] = "_BBN";
    if (AcpiEvaluateObject(bridge, bbnObject, nullptr, &addrBuf) == AE_OK)
    {
        return static_cast<int>(obj.Integer.Value);
    }
    else
    {
        std::unique_ptr<IoResource> pciSpace(makePciSpaceIoResource(pciAddr));
        if (pciSpace)
        {
            if ((pciSpace->in8(PciConfHeaderType) & 0x7F) == PciHeaderTypePciToPciBridge)
            {
                return pciSpace->in8(PciConfSecondaryBusNumber);
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
    }

private:
    struct CrsContext
    {
        ACPI_PCI_ROUTING_TABLE* m_routingTable;
        unsigned int m_bus;
    };
    
private:
    void processBridge(ACPI_HANDLE bridge, int level, int parentPciBus)
    {
        if (level >= 10)
        {
            println(L"ACPI: too long PCI bus depth");
            return;
        }

        PciAddress pciAddr;
        if (!getPciDeviceAddr(bridge, pciAddr, parentPciBus))
            return;

        int pciBus = getBridgePciBus(bridge, pciAddr);
        if (pciBus == -1)
            return;
        
        registerPciRoutingBus(pciBus, parentPciBus, pciAddr.m_device);

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
                {
                    addPciDeviceRouting(pciBus, (routingTable->Address >> 16), routingTable->Pin + 1, routingTable->SourceIndex);
                }
                else
                {
                    ACPI_HANDLE srcHadle;
                    if (AcpiGetHandle(bridge, routingTable->Source, &srcHadle) == AE_OK)
                    {
                        CrsContext ctx{routingTable, static_cast<unsigned int>(pciBus)};
                        static char crsObject[] = "_CRS";
                        AcpiWalkResources(srcHadle, crsObject, &PciIrqEnumerator::processCrs, &ctx);
                    }
                }
                tableBytes += routingTable->Length;
                routingTable = reinterpret_cast<ACPI_PCI_ROUTING_TABLE*>(tableBytes);
            }
        }

        if (rtblBuf.Pointer != nullptr)
            AcpiOsFree(rtblBuf.Pointer);

        ACPI_HANDLE childBridge = nullptr;
        while (AcpiGetNextObject(ACPI_TYPE_DEVICE, bridge, childBridge, &childBridge) == AE_OK)
            processBridge(childBridge, level + 1, pciBus);
    }

    static ACPI_STATUS processCrs(ACPI_RESOURCE* resource, void* context)
    {
        CrsContext* crsContext = (CrsContext*)context;
        ACPI_PCI_ROUTING_TABLE* routingTable = crsContext->m_routingTable;
        if (resource->Type == ACPI_RESOURCE_TYPE_IRQ) 
        {
            const ACPI_RESOURCE_IRQ* irq = &resource->Data.Irq;
            addPciDeviceRouting(crsContext->m_bus, (routingTable->Address >> 16), routingTable->Pin + 1, irq->Interrupts[routingTable->SourceIndex]);  
        }
        else if (resource->Type == ACPI_RESOURCE_TYPE_EXTENDED_IRQ) 
        {
            const ACPI_RESOURCE_EXTENDED_IRQ* irq = &resource->Data.ExtendedIrq;
            addPciDeviceRouting(crsContext->m_bus, (routingTable->Address >> 16), routingTable->Pin + 1, irq->Interrupts[routingTable->SourceIndex]);    
        }
        return AE_OK;
    }

    static ACPI_STATUS acpiProcessSystemBridge(ACPI_HANDLE bridge, UINT32, void* context, void**)
    {
        static_cast<PciIrqEnumerator*>(context)->processBridge(bridge, 0, 0);
        return AE_OK;
    }
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
