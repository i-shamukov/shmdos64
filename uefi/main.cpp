/*
   main.cpp
   Bootloader main
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

#include <kernel_params.h>
#include <cpu.h>
#include <vmem_utils.h>
#include <conout.h>
#include "config.h"
#include "UefiVideo.h"
#include "BootIo.h"
#include "paging.h"
#include "memory.h"
#include "KernelLoader.h"
#include "panic.h"

static void loadModules(const SystemConfig& config, KernelParams::Modules& params)
{
	params.m_count = 0;
	if (config.m_bootModules.empty())
		return;

	println(L"Loading boot modules...");
	auto& io = BootIo::getInstance();
	auto& allocator = KernelAllocator::getInstance();
	KernelParams::Modules::BootModule* pModules;
	allocator.allocPageAlign(pModules, config.m_bootModules.size());
	for (const kwstring& module : config.m_bootModules)
	{
		auto& curModule = pModules[params.m_count];
		if (module.size() >= sizeof(curModule.m_fileName))
		{
			println(L"Too long module file name ", module);
			continue;
		}

		kvector<char> moduleData;
		if (io.readFile(module.c_str(), 0x1000000, &moduleData))
		{
			void* pData = allocator.allocMemoryPageAlign(moduleData.size());
			kmemcpy(pData, moduleData.data(), moduleData.size());
			curModule.m_data = physToVirtual(pData);
			curModule.m_size = moduleData.size();
			kstrcpy(curModule.m_fileName, module.c_str());
			params.m_count++;
		}
	}

	params.m_modules = physToVirtual(pModules);
}

static void loadAcpi(KernelParams* params)
{
	static const EFI_GUID acpiGuid = {0x8868e871, 0xe4f1, 0x11d3,
		{0xbc, 0x22, 0x00, 0x80, 0xc7, 0x3c, 0x88, 0x81}};
	const EFI_SYSTEM_TABLE* sysTable = getSystemTable();
	params->m_acpiRsdpPhys = 0;
	for (UINTN idx = 0; idx < sysTable->NumberOfTableEntries; ++idx)
	{
		EFI_CONFIGURATION_TABLE& confTable = sysTable->ConfigurationTable[idx];
		if (kmemcmp(&confTable.VendorGuid, &acpiGuid, sizeof(acpiGuid)) == 0)
		{
			params->m_acpiRsdpPhys = reinterpret_cast<uintptr_t>(confTable.VendorTable);
			break;
		}
	}
	if (params->m_acpiRsdpPhys == 0)
		panic(L"ACPI tables not found");
}

void EfiMain()
{
	println(L"Startup SHM DOS64 UEFI Loader...");
	auto& allocator = KernelAllocator::getInstance();
	allocator.initMemoryMap();
	auto& video = UefiVideo::getInstance();
	KernelParams* params;
	allocator.allocPageAlign(params, 1);
	SystemConfig systemConfig;
	systemConfig.m_screenWidth = video.screenWidth();
	systemConfig.m_screenHeight = video.screenHeight();
	println(L"Loading sysem config...");
	if (!systemConfig.load())
		println(L"Error loading system config");

	print(L"Set video mode ", systemConfig.m_screenWidth, L'x', systemConfig.m_screenHeight, L' ');
	if (video.setVideoMode(systemConfig.m_screenWidth, systemConfig.m_screenHeight))
		println(L"OK");
	else
		println(L"FAIL");

	cpuTestCommands();

	KernelLoader kernel(systemConfig.m_kernelName, params);
	loadModules(systemConfig, params->m_modules);

	loadAcpi(params);
	if (systemConfig.m_fixFrameBufferMtrr)
		fixFrameBufferMtrr();
	dumpControlRegisters();
	video.debugInfo();

	if (!systemConfig.m_bootLogFileName.empty())
	{
		println(L"Saving boot console");
		video.saveConsoleToFile(systemConfig.m_bootLogFileName.c_str());
	}

	println(L"Run ", systemConfig.m_kernelName);
	video.storeParams(params->m_video);
	allocator.storeParams(params->m_physicalMemory);
	auto& paging = PagingManager64::getInstance();
	paging.storeParams(params->m_virtualMemory);
	if (getSystemTable()->BootServices->ExitBootServices(getImageHandle(), allocator.getMemoryKey()) != EFI_SUCCESS)
		panic(L"ExitBootServices failed");
	cpuDisableInterrupts();
	paging.setupVirtualMemory();
	allocator.setVirtualAddressMap();
	cpuSetDefaultControlRegisters();
	cpuInitFpu();
	kernel.run(physToVirtual(params));
	panic(L"Kerenl returned");
}
