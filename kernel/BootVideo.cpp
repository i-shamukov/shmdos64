/*
   BootVideo.cpp
   Kernel console
   SHM DOS64
   Copyright (c) 2022, Ilya Shamukov, ilya.shamukov@gmail.com
   
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
#include <klocale.h>
#include <kernel_export.h>
#include "BootVideo.h"

const uint32_t g_defaultScreenColor = 0x0000FF;

BootVideo::BootVideo()
{
	const auto& params = getKernelParams()->m_video;
	m_sreenBuffer = static_cast<uint32_t*>(params.m_sreenBuffer);
	m_symbolWidth = params.m_bootFontWidth;
	m_symbolHeight = params.m_bootFontHeight;
	m_screenWidth = params.m_screenWidth;
	m_screenHeight = params.m_screenHeight;
	m_screenPitch = params.m_screenPitch;
	m_consoleBuffer = params.m_consoleBuffer;
	m_consoleBufferSize = params.m_consoleBufferSize;
	m_consoleBufferPos = params.m_consoleBufferPos;
	m_curTextPosX = params.m_curTextPosX;
	m_curTextPosY = params.m_curTextPosY;
	m_textMaxX = m_screenWidth - (m_screenWidth % m_symbolWidth);
	m_textMaxY = m_screenHeight - (m_screenHeight % m_symbolHeight);
	m_lineSize = m_screenWidth / m_symbolWidth;
	m_maxLines = m_screenHeight / m_symbolHeight;
	m_fontBitmap = params.m_bootFontBitmap;
	m_symbolOffsetMultiple = (m_symbolWidth * m_symbolHeight) / 8;
	m_symbolHiMask = (1 << (m_symbolWidth - 1));
}

BootVideo& BootVideo::getInstance()
{
	static BootVideo instance;
	return instance;
}

void BootVideo::drawRectangle(int x, int y, int width, int height, const uint32_t Color)
{
	auto videoBufPtr = &m_sreenBuffer[y * m_screenPitch + x];
	int lineAdd = m_screenPitch - width;
	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++)
			*videoBufPtr++ = Color;
		videoBufPtr += lineAdd;
	}
}

void BootVideo::drawSymbol8x16(int x, int y, uint8_t symbol)
{
	static const uint32_t textColor = 0xFFFFFF;
	auto videoBufPtr = &m_sreenBuffer[y * m_screenPitch + x];
	auto bitmap = &m_fontBitmap[ static_cast<size_t>(symbol) * m_symbolOffsetMultiple];
	int lineAdd = m_screenPitch - m_symbolWidth;
	for (int y = 0; y < m_symbolHeight; y++)
	{
		for (int mask = m_symbolHiMask; mask > 0; mask >>= 1)
			*videoBufPtr++ = ((*bitmap & mask) != 0) ? textColor : g_defaultScreenColor;
		videoBufPtr += lineAdd;
		bitmap++;
	}
}

void BootVideo::putc(char symbol)
{
	if (m_consoleBufferPos < m_consoleBufferSize)
		m_consoleBuffer[m_consoleBufferPos++] = symbol;
	switch (symbol)
	{
	case '\n':
		m_curTextPosY += m_symbolHeight;
	case '\r':
		m_curTextPosX = 0;
		break;

	default:
		drawSymbol8x16(m_curTextPosX, m_curTextPosY, static_cast<uint8_t>(symbol));
		m_curTextPosX += m_symbolWidth;
		if (m_curTextPosX >= m_textMaxX)
		{
			m_curTextPosX = 0;
			m_curTextPosY += m_symbolHeight;
		}
		break;

	}
	if (m_curTextPosY >= m_textMaxY)
	{
		m_curTextPosY -= m_symbolHeight;
		drawRectangle(0, m_curTextPosY, m_textMaxX, m_symbolHeight, g_defaultScreenColor);
		shiftConsole();
	}
}

void BootVideo::shiftConsole()
{
	int line, cnt = 0;
	char *p = &m_consoleBuffer[m_consoleBufferPos - 1];
	if (*p == '\n')
		line = m_maxLines - 1;
	else
		line = m_maxLines - 2;
	for (;; p--)
	{
		if (*p == '\n')
		{
			cnt = 0;
			line--;
			if (line < 0)
				break;
		}
		else if (cnt == m_lineSize)
		{
			cnt = 0;
			line--;
		}
		else if (*p == '\r')
			continue;

		cnt++;
	}
	int col = 0;
	while (line < 0)
	{
		char symbol = *p++;
		switch (symbol)
		{
		case '\n':
			line++;
		case '\r':
			col = 0;
			break;

		default:
			if (++col >= m_lineSize)
			{
				col = 0;
				line++;
			}
			break;
		}
	}
	if (*p == '\n')
		p++;
	int cur_y = 0, cur_x = 0, end_y = m_textMaxY - m_symbolHeight;
	char old = '\0';
	while (cur_y < end_y)
	{
		char symbol = *p++;
		switch (symbol)
		{
		case '\n':
			if (old != '\r')
				drawRectangle(cur_x, cur_y, m_textMaxX - cur_x, m_symbolHeight, g_defaultScreenColor);
			cur_y += m_symbolHeight;

		case '\r':
			drawRectangle(cur_x, cur_y, m_textMaxX - cur_x, m_symbolHeight, g_defaultScreenColor);
			cur_x = 0;
			break;

		default:
			drawSymbol8x16(cur_x, cur_y, symbol);
			cur_x += m_symbolWidth;
			if (cur_x >= m_textMaxX)
			{
				cur_x = 0;
				cur_y += m_symbolHeight;
			}
			break;
		}
		old = symbol;
	}
}

KERNEL_SHARED void print(const char *str)
{
	auto& video = BootVideo::getInstance();
	while (*str != '\0')
		video.putc(*str++);
}

KERNEL_SHARED void print(const wchar_t *str)
{
	auto& video = BootVideo::getInstance();
	while (*str != '\0')
		video.putc(unicodeToChar(*str++));
}

KERNEL_SHARED void print(char symbol)
{
	BootVideo::getInstance().putc(symbol);
}

KERNEL_SHARED void print(wchar_t symbol)
{
	BootVideo::getInstance().putc(unicodeToChar(symbol));
}
