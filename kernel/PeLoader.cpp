/*
   PeLoader.cpp
   Kernel Portable Executable loader
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

#include <algorithm>
#include <kernel_params.h>
#include <VirtualMemoryManager.h>
#include <klocale.h>
#include <cpu.h>
#include <conout.h>
#include <KernelModule.h>
#include "panic.h"
#include "PeLoader.h"

static const size_t IntMaxModuleSize = 0x2000000;

static const wchar_t* g_errorStrings[] = 
{
	L"",
	L"Incorrect file format",
	L"Incorrect CPU architecture",
	L"Error allocate memory",
	L"Unknown reloc item",
	L"Could not find a dependent module ",
	L"Not found imported function ",
	L"Import type is not supported"
};

PeLoader::PeLoader(Process* process, const kstring& name)
	: m_process(process)
	, m_name(name)
{

}

PeLoader::~PeLoader()
{
	if (m_imageBasePtr != nullptr)
		m_process->vmm().free(m_imageBasePtr);
}

const kstring& PeLoader::name() const
{
	return m_name;
}

void* PeLoader::handle() const
{
	return reinterpret_cast<void*>(m_imageBase);
}

const void* PeLoader::procByName(const char* name) const
{
	auto entry = std::lower_bound(m_exportEntryes.begin(), m_exportEntryes.end(), name);
	if( (entry == m_exportEntryes.end()) || (kstrcmp(name, entry->m_funcName) != 0) )
		return nullptr;
	
	return entry->m_funcPtr;
}

void PeLoader::load()
{
	if (m_entryPoint != nullptr)
		reinterpret_cast<KernelModuleEntry>(m_entryPoint)(m_imageBasePtr, KernelModuleLoad, nullptr);
}

void PeLoader::unload()
{
	if (m_entryPoint != nullptr)
		reinterpret_cast<KernelModuleEntry>(m_entryPoint)(m_imageBasePtr, KernelModuleUnload, nullptr);
}

void PeLoader::onSystemMessage(int msgCode, int arg, void* ptr)
{
	if (m_entryPoint != nullptr)
	{
		KernelModuleMessage msg;
		msg.m_msg = msgCode;
		msg.m_arg = arg;
		msg.m_ptr = ptr;
		reinterpret_cast<KernelModuleEntry>(m_entryPoint)(m_imageBasePtr, KernelModuleSystemMsg, &msg);
	}
}

void PeLoader::loadNtHeaderFromMemory(uintptr_t addr)
{
	m_imageBase = addr;
	const IMAGE_DOS_HEADER* dosHeader = reinterpret_cast<const IMAGE_DOS_HEADER*>(m_imageBase);
	m_ntHeader = reinterpret_cast<const IMAGE_NT_HEADERS*>(m_imageBase + dosHeader->e_lfanew);
	m_imageSize = m_ntHeader->OptionalHeader.SizeOfImage;
}

void PeLoader::setLastError(ErrorCode code)
{
	m_lastErrorCode = code;
	m_lastErrorText = g_errorStrings[code];
}

void PeLoader::setLastError(ErrorCode code, const kwstring& exStr)
{
	setLastError(code);
	m_lastErrorText += exStr;
}

void PeLoader::setLastError(ErrorCode code, const kstring& exStr)
{
	kvector<wchar_t> buffer(exStr.size() + 1);
	charStrToUnicode(exStr.c_str(), buffer.data(), buffer.size());
	setLastError(code, buffer.data());
}

bool PeLoader::processExport()
{
	const uintptr_t exportRva = m_ntHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
	const size_t exportSize = m_ntHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size;
	if (exportRva == 0)
		return true;

	if ((exportRva + exportSize) > m_imageSize)
	{
		setLastError(ErrIncorrectFileFormat);
		return false;
	}

	const IMAGE_EXPORT_DIRECTORY* exportDir = reinterpret_cast<const IMAGE_EXPORT_DIRECTORY*>(m_imageBase + exportRva);
	const size_t ordBase = exportDir->Base;
	size_t numNames = exportDir->NumberOfNames;
	if (numNames >= 65536)
	{
		setLastError(ErrIncorrectFileFormat);
		return false;
	}

	const uintptr_t functionsRva = exportDir->AddressOfFunctions;
	const uintptr_t namesRva = exportDir->AddressOfNames;
	const uintptr_t ordRva = exportDir->AddressOfNameOrdinals;
	if( (functionsRva == 0) || (ordRva == 0) )
		return true;

	if(  ( functionsRva >= ( m_imageSize - numNames * sizeof(uint32_t) ) ) ||
		( namesRva >= ( m_imageSize - numNames * sizeof(uint32_t) ) ) ||
		( ordRva >= ( m_imageSize - numNames * sizeof(uint16_t) ) ) 
		)
	{
		setLastError(ErrIncorrectFileFormat);
		return false;
	}

	const uint32_t* pfunction = reinterpret_cast<const uint32_t*>(m_imageBase + functionsRva);
	const uint16_t* pord = reinterpret_cast<const uint16_t*>(m_imageBase + ordRva);
	const uint32_t* pname = (namesRva != 0) ? reinterpret_cast<const uint32_t*>(m_imageBase + namesRva) : nullptr;
	const char* funcName = "";
	while (numNames-- > 0)
	{
		if (namesRva != 0)
		{
			if (*pname >= m_imageSize)
			{
				setLastError(ErrIncorrectFileFormat);
				return false;
			}

			funcName = reinterpret_cast<const char*>(m_imageBase + *pname);
			ExportEntry entry;
			entry.m_funcName = funcName;
			entry.m_ord = ordBase + *pord;
			entry.m_funcPtr = reinterpret_cast<const void*>(m_imageBase + pfunction[*pord]);
			m_exportEntryes.push_back(entry);
			pname++;
		}
		pord++;
	}
	return true;
}

bool PeLoader::loadFromMemory(const void* moduleBytes, size_t size)
{
	const IMAGE_DOS_HEADER* dosHdr = static_cast<const IMAGE_DOS_HEADER*>(moduleBytes);
	if( ( size <= sizeof(*dosHdr) ) ||  
		(dosHdr->e_magic != IMAGE_DOS_SIGNATURE) ||
		(dosHdr->e_lfanew == 0) ||
		(dosHdr->e_lfanew > ( size - sizeof(IMAGE_NT_HEADERS) + sizeof(IMAGE_OPTIONAL_HEADER) ) )
		)
	{
		setLastError(ErrIncorrectFileFormat);
		return false;
	}

	loadNtHeaderFromMemory(reinterpret_cast<uintptr_t>(moduleBytes));
	if( (m_ntHeader->Signature != IMAGE_NT_SIGNATURE) ||
		(m_imageSize > IntMaxModuleSize) ||
		(m_ntHeader->OptionalHeader.SectionAlignment < PAGE_SIZE)
		)
	{
		setLastError(ErrIncorrectFileFormat);
		return false;
	}
	
	if (m_ntHeader->FileHeader.Machine != IMAGE_FILE_MACHINE_AMD64)
	{
		setLastError(ErrIncorrectArchitecture);
		return false;
	}
	
	m_imageBasePtr = m_process->vmm().alloc(m_imageSize, VMM_READWRITE);
	if (m_imageBasePtr == nullptr)
	{
		setLastError(ErrAlloc);
		return false;
	}
	
	if (!loadPeSectionsFromMemory(moduleBytes, size))
		return false;
	
	loadNtHeaderFromMemory(reinterpret_cast<uintptr_t>(m_imageBasePtr));
	if (m_ntHeader->OptionalHeader.AddressOfEntryPoint != 0)
	{
		if(m_ntHeader->OptionalHeader.AddressOfEntryPoint >= m_imageSize)
		{
			setLastError(ErrIncorrectFileFormat);
			return false;
		}

		m_entryPoint = reinterpret_cast<const void*>(m_imageBase + m_ntHeader->OptionalHeader.AddressOfEntryPoint);
	}
	return initImage();
}

bool PeLoader::loadPeSectionsFromMemory(const void* moduleBytes, size_t size)
{
	const IMAGE_DOS_HEADER* dosHdr = static_cast<const IMAGE_DOS_HEADER*>(moduleBytes);
	const size_t peHdrEnd = dosHdr->e_lfanew + sizeof(*m_ntHeader);
	kmemcpy(m_imageBasePtr, moduleBytes, peHdrEnd);
	const size_t nSections = m_ntHeader->FileHeader.NumberOfSections;
	const size_t segAlign = m_ntHeader->OptionalHeader.SectionAlignment;
	const uint8_t* pbModule = static_cast<const uint8_t*>(moduleBytes);
	m_sections = reinterpret_cast<const IMAGE_SECTION_HEADER*>(&pbModule[peHdrEnd]);
	uint8_t* pbImage = static_cast<uint8_t*>(m_imageBasePtr);

	for (size_t i = 0; i < nSections; i++)
	{
		const IMAGE_SECTION_HEADER& section = m_sections[i];
		size_t vSize = section.Misc.VirtualSize;
		const size_t vSizeRest = vSize % segAlign;
		if (vSizeRest > 0)
			vSize += segAlign - vSizeRest;
		if (section.VirtualAddress + vSize > m_imageSize)
		{
			setLastError(ErrIncorrectFileFormat);
			return false;
		}

		if (section.PointerToRawData != 0)
		{
			if (section.PointerToRawData + section.SizeOfRawData > size)
			{
				setLastError(ErrIncorrectFileFormat);
				return false;
			}

			uint8_t* s_base = &pbImage[section.VirtualAddress];
			if (section.SizeOfRawData >= section.Misc.VirtualSize)
				kmemcpy(s_base, &pbModule[section.PointerToRawData], section.Misc.VirtualSize);
			else
			{
				kmemcpy(s_base, &pbModule[section.PointerToRawData], section.SizeOfRawData);
				kmemset(&s_base[section.SizeOfRawData], 0, section.Misc.VirtualSize - section.SizeOfRawData);
			}
		}
		else if (section.Misc.VirtualSize > 0)
			kmemset(&pbImage[section.VirtualAddress], 0, section.Misc.VirtualSize);
	}
	return true;
}

bool PeLoader::initImage()
{
	if (!initRelocations())
		return false;
	
	if (!initImport())
		return false;
	
	if (!processExport())
		return false;
	
	protectSections();
	return true;
}

bool PeLoader::initRelocations()
{
	const uintptr_t relocOffset = m_ntHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress;	
	const uintptr_t deltaAddr = m_imageBase - m_ntHeader->OptionalHeader.ImageBase;
	if ((relocOffset == 0) || (deltaAddr == 0))
		return true;

	const uintptr_t imageEnd = m_imageBase + m_imageSize;
	const size_t relocSize = std::min<size_t>(m_ntHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size, m_imageSize - relocOffset);
	
	uintptr_t relocBase = m_imageBase + relocOffset;
	const uintptr_t memoryEnd = relocBase + relocSize;
	while (relocBase < memoryEnd)
	{
		const IMAGE_BASE_RELOCATION& reloc = *reinterpret_cast<IMAGE_BASE_RELOCATION*>(relocBase);
		if (reloc.VirtualAddress == 0)
			break;
		
		const uintptr_t curBlkPtr = m_imageBase + reloc.VirtualAddress;
		relocBase += sizeof(reloc);
		for (uintptr_t relocCnt = (reloc.SizeOfBlock - sizeof(reloc)) / 2; relocCnt-- > 0; )
		{
			uint16_t r = *reinterpret_cast<uint16_t*>(relocBase);
			uintptr_t curItemPtr = curBlkPtr + (r & 0x0FFF);
			if (curItemPtr >= (imageEnd - sizeof(uintptr_t) ) )
			{
				setLastError(ErrIncorrectFileFormat);
				return false;
			}
			
			switch (r >> 12)
			{
			case 0:
				break;

			case 3:
				*reinterpret_cast<uint32_t*>(curItemPtr) += static_cast<uint32_t>(deltaAddr);
				break;

			case 10:
				*reinterpret_cast<uint64_t*>(curItemPtr) += deltaAddr;
				break;

			default:
				setLastError(ErrUnknownReloc);
				return false;
			}
			relocBase += 2;
		}
	}
	return true;
}

bool PeLoader::initImport()
{
	const uintptr_t importRva = m_ntHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;
	const uintptr_t importSize = m_ntHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size;
	if( (importSize >= IntMaxModuleSize) || (importRva + importSize) > m_imageSize)
	{
		setLastError(ErrIncorrectFileFormat);
		return false;
	}
	
	const IMAGE_IMPORT_DESCRIPTOR* importDescriptor = reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR*>(m_imageBase + importRva);
	size_t descriptorCount = importSize / sizeof(*importDescriptor);
	while (descriptorCount-- > 0)
	{
		if (importDescriptor->Characteristics == 0)
			break;

		const char *moduleName = reinterpret_cast<char*>(m_imageBase + importDescriptor->Name);
		const AbstractModule* module = m_process->moduleByName(moduleName);
		if (module == nullptr)
		{
			setLastError(ErrImortModule, moduleName);
			return false;
		}
		
		if( (importDescriptor->FirstThunk >= m_imageSize) || (importDescriptor->OriginalFirstThunk >= m_imageSize) )
		{
			setLastError(ErrIncorrectFileFormat);
			return false;
		}

		uintptr_t* addrTable = reinterpret_cast<uintptr_t*>(m_imageBase + importDescriptor->FirstThunk);
		const uintptr_t* lookupTable = (importDescriptor->OriginalFirstThunk == 0) ? addrTable : reinterpret_cast<uintptr_t*>(m_imageBase + importDescriptor->OriginalFirstThunk);
		const uintptr_t* maxAddr = reinterpret_cast<uintptr_t*>(m_imageBase + m_imageSize - sizeof(uintptr_t));
		for (; *addrTable != 0; addrTable++, lookupTable++)
		{
			if ((addrTable >= maxAddr) || (lookupTable >= maxAddr))
			{
				setLastError(ErrIncorrectFileFormat);
				return false;
			}

			const uintptr_t lookup = *lookupTable;
			if( (lookup & (1ULL << (8 * sizeof(uintptr_t) - 1))) != 0)
			{
				setLastError(ErrImportType, L"(ordinal)");
				return false;
			}
			else
			{
				
				const char *funcName = reinterpret_cast<char*>(m_imageBase + lookup + 2);
				const void *func = module->procByName(funcName);
				if (func == nullptr)
				{
					kstring str = moduleName;
					str += ':';
					str += funcName;
					setLastError(ErrImortFunction, str);
					return false;
				}

				*addrTable = reinterpret_cast<uintptr_t>(func);
			}
		}
		++importDescriptor;
	}

	if (m_ntHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT].Size != 0) 
	{
		setLastError(ErrImportType);
		return false;
	}
	
	if (m_ntHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT].Size != 0) 
	{
		setLastError(ErrImportType);
		return false;
	}

	return true;
}

void PeLoader::protectSections()
{
	const size_t nSections = m_ntHeader->FileHeader.NumberOfSections;
	const size_t segAlign = m_ntHeader->OptionalHeader.SectionAlignment;
	VirtualMemoryManager& vmm = m_process->vmm();
	for (size_t i = 0; i < nSections; i++)
	{
		const IMAGE_SECTION_HEADER& section = m_sections[i];
		size_t vSize = section.Misc.VirtualSize;
		const size_t vSizeRest = vSize % segAlign;
		if (vSizeRest > 0)
			vSize += segAlign - vSizeRest;
		
		vmm.setPagesFlags(reinterpret_cast<void*>(m_imageBase + section.VirtualAddress), vSize, VMM_READONLY);
	}
}

void PeLoader::initKernel()
{
	const KernelParams* params = getKernelParams();
	Process* kernelProcess = &Process::kernel();
	static PeLoader kernelModule(kernelProcess, "kernel.exe");
	kernelModule.loadNtHeaderFromMemory(params->m_imageBase);
	if (!kernelModule.processExport())
	{
		PANIC((L"kernel export failed " + kernelModule.lastErrorText()).c_str());
	}
	kernelProcess->addModule(&kernelModule);
	VirtualMemoryManager& vmm = kernelProcess->vmm();
	for (size_t idx = 0; idx < params->m_modules.m_count; ++idx)
	{
		const KernelParams::Modules::BootModule& module = params->m_modules.m_modules[idx];
		constexpr size_t maxFileSize = sizeof(module.m_fileName) / sizeof(*module.m_fileName);
		char fileName[maxFileSize];
		unicodeToCharStr(module.m_fileName, fileName, maxFileSize);
		print(L"Loading kernel module ",  module.m_fileName, L"... ");
		PeLoader* kernelModule = new PeLoader(kernelProcess, fileName);
		if (kernelModule->loadFromMemory(module.m_data, module.m_size))
		{
			println(L"OK");
			kernelProcess->addModule(kernelModule);
		}
		else
		{
			println(L"FAILED");
			println(kernelModule->lastErrorText());
			delete kernelModule;
		}
		vmm.freeRamPages(module.m_data, module.m_size, false);
	}
}

void PeLoader::loadKernelModules()
{
	initKernel();
}
