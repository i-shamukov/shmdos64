/*
   Process.cpp
   Kernel power interface
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

#include <atomic>
#include <power.h>
#include <cpu.h>
#include <KernelModule.h>
#include <AbstractDevice.h>
#include <AbstractDriver.h>
#include "idt.h"
#include "panic.h"
#include "LocalApic.h"

static PowerFunctions g_systemPowerFuntionDef;
static PowerFunctions* g_systemPowerFuntion = &g_systemPowerFuntionDef;

PowerFunctions::PowerFunctions()
{
}

PowerFunctions::~PowerFunctions()
{
}

void PowerFunctions::reset()
{
    for(int i = 0; i < 100000; i++)
		if(!(inportb(0x64) & 0x02))
			break;
	outportb(0x64, 0xFE);
	outportb(0x92, 0xFF);
    cpuStop();
}

void PowerFunctions::prepareShutdown()
{
    cpuStop();
}

void PowerFunctions::shutdown()
{
    cpuStop();
}

void PowerFunctions::sleep(int)
{
}

PowerFunctions* PowerFunctions::get()
{
    return g_systemPowerFuntion;
}

void PowerFunctions::set(PowerFunctions* pf)
{
    g_systemPowerFuntion = ((pf != nullptr) ? pf : &g_systemPowerFuntionDef);
}


class PowerButtonDevice: public AbstractDevice
{
public:
    static PowerButtonDevice& instance()
    {
        static PowerButtonDevice dev;
        return dev;
    }

    void push()
    {
        postInterruptMessage(0, 0, nullptr);
    }

    void onInterruptMessage(int, int, void*) override
    {
        KernelPower::systemShotdown();
    }

private:
    PowerButtonDevice()
        : AbstractDevice(DeviceClass::System, L"Power Button", AbstractDevice::root(), AbstractDriver::kernel())
    {

    }
    PowerButtonDevice(const PowerFunctions&) = delete;
    PowerButtonDevice(PowerButtonDevice&&) = delete;
    PowerButtonDevice& operator=(const PowerButtonDevice&) = delete;
};


// shutdown() can be called only on Boot CPU
INTERRUPT_HANDLER(systemShutdownHandler, "")
{
	(void)state;
    LocalApic::system().sendBroadcastIpi(CPU_STOP_VECTOR);
    PowerFunctions::get()->shutdown();
    cpuStop();
    PANIC(L"Shutdown function didn't work");
}

namespace KernelPower
{
    void powerButtonHandler()
    {
        PowerButtonDevice::instance().push();
    }

    void systemShotdown()
    {
        static std::atomic<bool> shutdownProccess{false};
        if (shutdownProccess.load(std::memory_order_acquire))
            return;
        
        shutdownProccess.store(true, std::memory_order_relaxed);
        makeKernelModuleMessage(SystemEventTermintate);
        makeKernelModuleMessage(SystemEventShotdown);
        PowerFunctions::get()->prepareShutdown();
        LocalApic::system().sendIpi(BOOT_CPU_ID, CPU_SYSTEM_SHUTDOWN_VECTOR);
        for( ; ; )
            cpuHalt();
    }

    void init()
    {
        SystemIDT::setHandler(CPU_SYSTEM_SHUTDOWN_VECTOR, &systemShutdownHandler, true);
    }
}
