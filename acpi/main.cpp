/*
   main.cpp
   ACPI module main
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

#include <AbstractDevice.h>
#include <power.h>
#include "AcpiInit.h"
#include "PciIrqRouting.h"

static UINT32 acpiPowerButtonHandler(void*)
{
    KernelPower::powerButtonHandler();
    return 0;
}

void onModuleLoad()
{
    class AcpiPowerFunctions: public PowerFunctions
    {
    public:
        void prepareShutdown() override
        {
            AcpiEnterSleepStatePrep(5);
        }

        void shutdown() override
        {
            AcpiEnterSleepState(5);
        }
    };

    if (initializeFullAcpica() != AE_OK)
        return;

    static AcpiPowerFunctions pf;
    PowerFunctions::set(&pf);
    AcpiInstallFixedEventHandler(ACPI_EVENT_POWER_BUTTON, &acpiPowerButtonHandler, nullptr);
    AcpiEnableEvent(ACPI_EVENT_POWER_BUTTON, 0);
    initPciIrq();
}

void onModuleUnload()
{
    PowerFunctions::set(nullptr);
    AcpiDisableEvent(ACPI_EVENT_POWER_BUTTON, 0);
    AcpiRemoveFixedEventHandler(ACPI_EVENT_POWER_BUTTON, &acpiPowerButtonHandler);
}

void onSystemMessage(int, int, void*)
{

}


