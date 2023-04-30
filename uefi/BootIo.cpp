/*
   BootIo.cpp
   Read & Write files for bootloader
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

#include <conout.h>
#include "BootIo.h"
#include "panic.h"
#include "memory.h"

const static wchar_t* g_systemFolerName = L"sys64";

struct FileGuard
{
	~FileGuard()
	{
		if (m_handle != nullptr)
			m_handle->Close(m_handle);
	}
	EFI_FILE_PROTOCOL* m_handle = nullptr;
};

BootIo::BootIo()
{
	static EFI_GUID ipg = EFI_LOADED_IMAGE_PROTOCOL_GUID;
	static EFI_GUID fpg = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;
	const auto bs = getSystemTable()->BootServices;

	EFI_LOADED_IMAGE_PROTOCOL* image;
	if (bs->HandleProtocol(getImageHandle(), &ipg, (void**) &image) != EFI_SUCCESS)
		panic(L"Error receiving EFI_LOADED_IMAGE_PROTOCOL");

	EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* fs;
	if (bs->HandleProtocol(image->DeviceHandle, &fpg, (void**) &fs) != EFI_SUCCESS)
		panic(L"Error receiving EFI_SIMPLE_FILE_SYSTEM_PROTOCOL");

	EFI_FILE_PROTOCOL* root;
	if (fs->OpenVolume(fs, &root) != EFI_SUCCESS)
		panic(L"Error openig boot volume");

	if (root->Open(root, &m_sysFolder, g_systemFolerName, EFI_FILE_MODE_READ, 0) != EFI_SUCCESS)
		panic(L"Error openig system folder");
}

bool BootIo::readFile(const wchar_t* wFileName, size_t maxSize, kvector<char>* dataPtr, void** pagesPtr, size_t* dataSize)
{
	const wchar_t* failStr = L"FAIL\n";
	print(L"Read file ", wFileName, L"... ");
	FileGuard file;
	if (m_sysFolder->Open(m_sysFolder, &file.m_handle, wFileName, EFI_FILE_MODE_READ, 0) != EFI_SUCCESS)
	{
		println(failStr, L"Error open file");
		return false;
	}
	file.m_handle->SetPosition(file.m_handle, static_cast<UINT64>(-1));
	UINT64 fileSize = 0;
	file.m_handle->GetPosition(file.m_handle, &fileSize);
	file.m_handle->SetPosition(file.m_handle, 0);
	if (dataSize != nullptr)
		*dataSize = fileSize;
	if (fileSize > maxSize)
	{
		println(failStr, L"File too large");
		return false;
	}

	void *pData;
	if (dataPtr != nullptr)
	{
		auto& vData = *dataPtr;
		vData.resize(fileSize);
		pData = static_cast<void*>(vData.data());
	}
	else
	{
		pData = KernelAllocator::getInstance().allocMemoryPageAlign(fileSize);
		*pagesPtr = pData;
	}

	UINT64 bytesRead = fileSize;
	if ((file.m_handle->Read(file.m_handle, &bytesRead, pData) != EFI_SUCCESS)
		|| (fileSize != bytesRead))
	{
		println(failStr, L"Error read file");
		return false;
	}

	println("OK");
	return true;
}

bool BootIo::writeFile(const wchar_t* wFileName, const char* data, size_t size)
{
	FileGuard file;
	if (m_sysFolder->Open(m_sysFolder, &file.m_handle, wFileName, EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE, 0) != EFI_SUCCESS)
		return false;

	UINT64 bytesWrite = size;
	if (file.m_handle->Write(file.m_handle, &bytesWrite, static_cast<void*>(const_cast<char*>(data))) != EFI_SUCCESS)
		return false;

	return (bytesWrite == size);
}

BootIo& BootIo::getInstance()
{
	static BootIo loader;
	return loader;
}
