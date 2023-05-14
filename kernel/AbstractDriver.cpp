/*
   AbstractDrivier.cpp
   Abstract drvier implementation
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

#include "AbstractDriver_p.h"

AbstractDriverPrivate::AbstractDriverPrivate(const wchar_t* name)
   : m_name(name)
{

}

AbstractDriverPrivate::~AbstractDriverPrivate()
{

}

void AbstractDriverPrivate::addDevice(AbstractDevice* device)
{
   klock_guard lock(m_devListMtx);
   m_devList.emplace_back(device);
}

void AbstractDriverPrivate::removeDevice(AbstractDevice* device)
{
   klock_guard lock(m_devListMtx);
   for (auto it = m_devList.begin(); it != m_devList.end(); ++it)
   {
      if (it->get() == device)
      {
         it->release();
         m_devList.erase(it);
         break;
      }
   }
}

AbstractDriver::AbstractDriver(const wchar_t* name)
   : m_private(new AbstractDriverPrivate(name))
{

}

AbstractDriver::~AbstractDriver()
{
   delete m_private;
}

AbstractDriver* AbstractDriver::kernel()
{
   static AbstractDriver kernelDriver(L"Kernel");
   return &kernelDriver;
}
