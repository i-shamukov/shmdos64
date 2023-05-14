/*
   AbstractDevice.h
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
#include <functional>
#include <common_types.h>
#include <kernel_export.h>

enum class DeviceClass
{
	System,
	PciBus,
	StorageContoller,
	Storage
};

enum class IoResourceType
{
	IoSpace,
	MmioSpace,
	PciSpace
};

class AbstractDevicePrivate;
class IoResource;
class AbstractDriver;
class KERNEL_SHARED AbstractDevice
{
public:
	AbstractDevice(DeviceClass deviceClass, const wchar_t* name, AbstractDevice* parent, AbstractDriver* driver);
	virtual ~AbstractDevice();
	AbstractDevice* parent();
	virtual void onInterruptMessage(int arg1, int arg2, void* data);

	static AbstractDevice* root();

protected:
	virtual IoResource* io(uint64_t base, size_t size, IoResourceType type);
	bool installInterruptHandler(unsigned int irq);
	void removeInterruptHandler();
	void eoi();
	void postInterruptMessage(int arg1, int arg2, void* data);
	void removeChildIf(const std::function<bool(AbstractDevice*)>& pred);

private:
	AbstractDevice(const AbstractDevice&) = delete;
	AbstractDevice(AbstractDevice&&) = delete;
    AbstractDevice& operator=(const AbstractDevice&) = delete;
	virtual bool interruptHandler();

private:
	AbstractDevicePrivate* m_private;

	friend class AbstractDevicePrivate;
};