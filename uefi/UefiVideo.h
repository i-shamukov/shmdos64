/*
   UefiVideo.h
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
#include "efi.h"
#include <kvector.h>
#include <limits>
#include <kernel_params.h>

class UefiVideo
{
public:
	static UefiVideo& getInstance();
	void putc(char symbol);
	bool saveConsoleToFile(const wchar_t* fileName);
	bool setVideoMode(int width, int height);
	void storeParams(KernelParams::Video& params);
	void debugInfo();
	uintptr_t frameBuffer() const;

	int screenWidth() const
	{
		return m_screenWidth;
	}

	int screenHeight() const
	{
		return m_screenHeight;
	}

private:
	UefiVideo();
	UefiVideo(const UefiVideo&) = delete;
	UefiVideo(UefiVideo&&) = delete;

	void drawRectangle(int x, int y, int width, int height, const EFI_GRAPHICS_OUTPUT_BLT_PIXEL& Color);
	void drawSymbol8x16(int x, int y, uint8_t symbol);

	void shiftConsole();
	void parseEdid();

public:
	static const int m_symbolWidth = 8;
	static const int m_symbolHeight = 16;
	static const int m_defaultScreenWidth = 800;
	static const int m_defaultScreenHeight = 600;

private:
	int m_screenWidth = m_defaultScreenWidth;
	int m_screenHeight = m_defaultScreenHeight;
	int m_screenPitch = m_defaultScreenWidth;
	unsigned m_currentMode = std::numeric_limits<unsigned>::max();

	EFI_GRAPHICS_OUTPUT_PROTOCOL* m_protocol = nullptr;
	kvector<EFI_GRAPHICS_OUTPUT_BLT_PIXEL> m_symbolBuffer{m_symbolWidth * m_symbolHeight};
	char* m_consoleBuffer;
	static const size_t m_consoleBufferSize = 0x10000;
	size_t m_consoleBufferPos = 0;
	int m_curTextPosX = 0;
	int m_curTextPosY = 0;
	int m_textMaxX = 0;
	int m_textMaxY = 0;
	int m_lineSize = 0;
	int m_maxLines = 0;
};
