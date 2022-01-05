/*
   PeLoader.h
   Kernel header
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
#include <pe64.h>
#include "Process.h"
#include "AbstractModule.h"

class PeLoader : public AbstractModule
{
public:
	enum ErrorCode
	{
		ErrNone = 0,
		ErrIncorrectFileFormat,
		ErrIncorrectArchitecture,
		ErrAlloc,
		ErrUnknownReloc,
		ErrImortModule,
		ErrImortFunction,
		ErrImportType
	};

public:
	~PeLoader();
	ErrorCode lastErrorCode() const
	{
		return m_lastErrorCode;
	}
	
	const kwstring& lastErrorText() const
	{
		return m_lastErrorText;
	}

	const kstring& name() const override;
	void* handle() const override;
	const void* procByName(const char* name) const override;
	void load() override;
	void unload() override;
	void onSystemMessage(int, int, void*) override;

	static void loadKernelModules();
	
private:
	struct ExportEntry
	{
		const char *m_funcName;
		int m_ord;
		const void *m_funcPtr;
		bool operator < (const char* name) const
		{
			return (kstrcmp(m_funcName, name) < 0);
		}
	};

private:
	PeLoader(Process* process, const kstring& name);
	void loadNtHeaderFromMemory(uintptr_t addr);
	bool processExport();
	void setLastError(ErrorCode code);
	void setLastError(ErrorCode code, const kwstring& exStr);
	void setLastError(ErrorCode code, const kstring& exStr);
	bool loadFromMemory(const void* moduleBytes, size_t size);
	bool loadPeSectionsFromMemory(const void* moduleBytes, size_t size);
	bool initImage();
	bool initRelocations();
	bool initImport();
	void protectSections();
	static void initKernel();
	
private:
	Process* m_process;
	const kstring m_name;
	kwstring m_lastErrorText;
	ErrorCode m_lastErrorCode = ErrNone;
	void* m_imageBasePtr = nullptr;
	uintptr_t m_imageBase = 0;
	size_t m_imageSize = 0;
	const IMAGE_NT_HEADERS* m_ntHeader = nullptr;
	kvector<ExportEntry> m_exportEntryes;
	const void* m_entryPoint = nullptr;
	const IMAGE_SECTION_HEADER* m_sections = nullptr;
};



