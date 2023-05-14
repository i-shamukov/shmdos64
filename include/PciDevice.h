/*
   PciDevice.h
   Kernel shared header
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
#include <functional>
#include <AbstractDevice.h>
#include <AbstractDriver.h>
#include <common_types.h>
#include <kvector.h>
#include <pci.h>

#ifdef PCIDEVICE_EXPORT
    #define PCIDEVICE_SHARED __declspec(dllexport)
#else
    #define PCIDEVICE_SHARED
#endif

struct PciDeviceInfo
{
    PciAddress m_pciAddress;
    unsigned int m_vendorId;
    unsigned int m_deviceId;
    unsigned int m_classCode;
    unsigned int m_subclassCode;
    unsigned int m_progInterface;
    unsigned int m_irq;
    unsigned int m_pin;
    uint32_t m_bar[6];
};

struct PciDeviceState;
class PciDeviceDriver;
class PCIDEVICE_SHARED PciDevice: public AbstractDevice
{
public:
    PciDevice(DeviceClass deviceClass, const wchar_t* name, PciDeviceState* pciState, PciDeviceDriver* driver);
    virtual ~PciDevice();

protected:
    IoResource* io(unsigned int barIndex, size_t size);

private:
    PciDeviceState* m_pciState;
};

class PciDeviceDriverPrivate;
class PCIDEVICE_SHARED PciDeviceDriver: public AbstractDriver
{
public:
    PciDeviceDriver(const wchar_t* name);
    virtual ~PciDeviceDriver();
    virtual PciDevice* deviceProbe(const PciDeviceInfo& info, IoResource* pciConf, PciDeviceState* pciState);

protected:
    bool addCompatibleClassCode(unsigned int classCode, unsigned int subclassCode, const kvector<unsigned int>& progInterface);  

private:
    PciDeviceDriverPrivate* m_private;
};



