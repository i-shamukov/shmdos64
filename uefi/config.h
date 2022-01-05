/*
   config.h
   Header for EFI BootLoader
   SHM DOS64
   Copyright (c) 2022, Ilya Shamukov <ilya.shamukov@gmail.com>
   
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

#include <kstring.h>
#include <kvector.h>

class SystemConfig
{
public:
	bool load();

public:
	char* m_configData = nullptr;
	size_t m_configSize = 0;
	kvector<kwstring> m_bootModules;
	kwstring m_kernelName = L"kernel.exe";
	int m_screenWidth = 0;
	int m_screenHeight = 0;
	bool m_fixFrameBufferMtrr = false;
	kwstring m_bootLogFileName;

private:
	void processParam(const kwstring& param, const kvector<kwstring>& values);
	void parseFile();
};
