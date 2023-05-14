/*
   AbstractDevice.cpp
   Abstract device implementation
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

#include "panic.h"
#include "InterruptQueuePool.h"
#include "AbstractDevice_p.h"
#include "TaskManager.h"
#include "AbstractDriver_p.h"

AbstractDevice::AbstractDevice(DeviceClass deviceClass, const wchar_t* name, AbstractDevice* parent, AbstractDriver* driver)
	: m_private(new AbstractDevicePrivate(this, deviceClass, name, parent, driver))
{

}

AbstractDevice::~AbstractDevice()
{
	delete m_private;
}

AbstractDevice* AbstractDevice::parent()
{
	return m_private->m_parent;
}

IoResource* AbstractDevice::io(uint64_t base, size_t size, IoResourceType type)
{
	std::unique_ptr<IoResource> res;
	if (type == IoResourceType::IoSpace)
	{
		res = std::make_unique<IoSpace>(static_cast<uint16_t>(base), static_cast<uint16_t>(size));
	}
	else if (type == IoResourceType::MmioSpace)
	{
		res = std::make_unique<MmioSpace>(base, size);
	}
	else
	{
		PANIC(L"Undefefined resource type");
	}
	IoResource* ptr = res.get();
	m_private->m_ioResources.emplace_back(std::move(res));
	return ptr;
}

AbstractDevice* AbstractDevice::root()
{
	static AbstractDevice rootDevice(DeviceClass::System, L"System", nullptr, AbstractDriver::kernel());
	return &rootDevice;
}

bool AbstractDevice::interruptHandler()
{
	return false;
}

void AbstractDevice::onInterruptMessage(int, int, void*)
{

}

void AbstractDevice::postInterruptMessage(int arg1, int arg2, void* data)
{
	m_private->m_interruptQueuePool->postMessage(this, arg1, arg2, data);
}

bool AbstractDevice::installInterruptHandler(unsigned int irq)
{
	if (m_private->m_interruptHandlerId != 0)
		return false;

	const unsigned int id = ExternalInterrupts::system().installHandler(irq, &AbstractDevicePrivate::interruptHandler, this);
	if (id == 0)
		return false;

	m_private->m_interruptHandlerId = id;
	return true;
}

void AbstractDevice::removeInterruptHandler()
{
	m_private->removeInterruptHandler();
}

void AbstractDevice::eoi()
{
	ExternalInterrupts::system().eoi(m_private->m_irq);
}

void AbstractDevice::removeChildIf(const std::function<bool(AbstractDevice*)>& pred)
{
	klist<AbstractDevice*>& childrens = m_private->m_childrens;
	for (auto it = childrens.begin(); it != childrens.end(); )
	{
		AbstractDevice* dev = *it;
		if (pred(dev))
		{
			it = childrens.erase(it);
			dev->m_private->m_isManualRemoved = true;
			delete dev;
		}
		else
		{
			++it;
		}
	}
}

AbstractDevicePrivate::AbstractDevicePrivate(AbstractDevice* obj, DeviceClass deviceClass, const wchar_t* name, AbstractDevice* parent, AbstractDriver* driver)
	: m_deviceClass(deviceClass)
	, m_name(name)
	, m_obj(obj)
	, m_parent(parent)
	, m_driver(driver)
{
	m_interruptQueuePool = (TaskManager::system() != nullptr) ? &InterruptQueuePool::system() : nullptr;
	m_driver->m_private->addDevice(obj);
}

AbstractDevicePrivate::~AbstractDevicePrivate()
{
	m_isDestroyed = true;
	m_driver->m_private->removeDevice(m_obj);
	removeInterruptHandler();

	if ((m_parent != nullptr) && !m_parent->m_private->m_isDestroyed && !m_isManualRemoved)
		m_parent->m_private->m_childrens.remove_first(m_obj);

	for (AbstractDevice* child : m_childrens)
		delete child;
}

void AbstractDevicePrivate::removeInterruptHandler()
{
	if (m_interruptHandlerId != 0)
	{
		ExternalInterrupts::system().deleteHandler(m_irq, m_interruptHandlerId);
		m_interruptHandlerId = 0;
	}
}

bool AbstractDevicePrivate::interruptHandler(void* obj)
{
	return static_cast<AbstractDevice*>(obj)->interruptHandler();
}
