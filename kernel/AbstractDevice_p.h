/*
   AbstractDevice_p.h
   Kernel header
   SHM DOS64
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
#include <memory>
#include <kvector.h>
#include <kstring.h>
#include <klist.h>
#include <AbstractDevice.h>
#include "IoResourceImpl.h"
#include "ExternalInterrupts.h"

class AbstractDevicePrivate
{
public:
	AbstractDevicePrivate(AbstractDevice* obj, DeviceClass deviceClass, const wchar_t* name, AbstractDevice* parent, AbstractDriver* m_driver);
	~AbstractDevicePrivate();
	void removeInterruptHandler();

private:
	AbstractDevicePrivate(const AbstractDevicePrivate&) = delete;
	AbstractDevicePrivate(AbstractDevicePrivate&&) = delete;

	static bool interruptHandler(void* obj);

private:
	const DeviceClass m_deviceClass;
	const kwstring m_name;
	AbstractDevice* m_obj;
	AbstractDevice* m_parent;
	bool m_isDestroyed = false;
	bool m_isManualRemoved = false;
	klist<AbstractDevice*> m_childrens;
	kvector<std::unique_ptr<IoResource>> m_ioResources;
	unsigned int m_irq;
	unsigned int m_interruptHandlerId = 0;
	InterruptQueuePool* m_interruptQueuePool = nullptr;
	AbstractDriver* m_driver;

	friend class AbstractDevice;
};
