/*
   PciRoot.h
   Headers for PCI driver
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
#include <AbstractDevice.h>
#include <pci.h>
#include <kvector.h>

class PciRoot : public AbstractDevice
{
public:
	static PciRoot& instance();
	IoResource* makeIoResource(const PciAddress& pciAddr);

private:
	PciRoot();
	PciRoot(const PciRoot&) = delete;
	PciRoot(PciRoot&&) = delete;
	PciRoot& operator=(const PciRoot&) = delete;
	
private:
	IoResource* m_io;
	const kvector<SystemEnhancedPciSegment>& m_exSegments;
};
