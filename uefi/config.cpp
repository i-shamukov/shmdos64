/*
   config.cpp
   Kernel config parser for bootloader
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

#include <klocale.h>
#include "config.h"
#include "BootIo.h"

static bool isalpha(char c)
{
	return ( ((c >= 'a') && (c <= 'z')) || ((c >= 'A') && (c <= 'Z')));
}

static bool isdigit(char c)
{
	return ( (c >= '0') && (c <= '9'));
}

static bool isalnum(char c)
{
	return isalpha(c) || isdigit(c) || (c == '_');
}

bool strToIntEx(const wchar_t** str, unsigned* value)
{
	unsigned val = 0;
	const wchar_t* strPtr = *str;
	if (!isdigit(*strPtr))
		return false;

	do
	{
		val *= 10;
		val += *strPtr - L'0';
		strPtr++;
	}
	while (isdigit(*strPtr));
	*str = strPtr;
	*value = val;
	return true;
}

void SystemConfig::processParam(const kwstring& param, const kvector<kwstring>& values)
{
	if (kstricmp(param.c_str(), L"kernel") == 0)
	{
		const kwstring& kernel = values.front();
		if (!kernel.empty())
			m_kernelName = kernel;
		return;
	}

	if (kstricmp(param.c_str(), L"bootload") == 0)
	{
		for (const kwstring& module : values)
			if (!module.empty())
				m_bootModules.push_back(module);
		return;
	}

	if (kstricmp(param.c_str(), L"screenmode") == 0)
	{
		unsigned w, h;
		const wchar_t* value = values.front().c_str();
		if (!strToIntEx(&value, &w))
			return;

		if (*value++ != L'x')
			return;

		if (!strToIntEx(&value, &h))
			return;

		m_screenWidth = w;
		m_screenHeight = h;
		return;
	}

	if (kstricmp(param.c_str(), L"bootlog") == 0)
	{
		m_bootLogFileName = values.front();
		return;
	}
}

void SystemConfig::parseFile()
{
	int state = 0;
	bool isRead = true;
	char c;
	kwstring paramName;
	kvector<kwstring> paramValues(1);

	const char* ptr = m_configData;
	const char* end = &m_configData[m_configSize];
	for ( ; ; )
	{
		if (isRead)
		{
			if (ptr >= end)
				break;
			c = *ptr++;
		}
		else
			isRead = true;

		switch (state)
		{
		case 0:
			if (c == ';')
				state = 1;
			if (isalnum(c))
			{
				paramName += charToUnicode(c);
				state = 2;
			}
			break;
		case 1:
			if ((c == '\r') || (c == '\n'))
			{
				paramName.clear();
				paramValues.resize(1);
				paramValues.back().clear();
				state = 0;
			}
			break;
		case 2:
			if (isalnum(c))
				paramName += charToUnicode(c);
			else if ((c == ' ') || (c == '\t'))
				state = 3;
			else if (c == '=')
				state = 4;
			else
			{
				isRead = false;
				state = 1;
			}
			break;
		case 3:
			if (c == '=')
				state = 4;
			else if ((c != ' ') && (c != '\t'))
			{
				isRead = false;
				state = 1;
			}
			break;
		case 4:
			if (isalnum(c))
			{
				isRead = false;
				state = 6;
				break;
			}
			else
			{
				switch (c)
				{
				case '"':
					state = 5;
					break;
				case ' ':
				case '\t':
					break;
				case '\r':
				case '\n':
					paramValues.resize(1);
					paramValues.back().clear();
					isRead = false;
					state = 8;
					break;
				default:
					isRead = false;
					state = 1;
					break;
				}
			}
			break;
		case 5:
			switch (c)
			{
			case '\r':
			case '\n':
				isRead = false;
				state = 1;
				break;
			case '"':
				paramValues.push_back(kwstring());
				state = 7;
				break;
			default:
				paramValues.back() += charToUnicode(c);
				break;
			}
			break;
		case 6:
			switch (c)
			{
			case '\r':
			case '\n':
				paramValues.push_back(kwstring());
				isRead = false;
				state = 8;
				break;
			case ',':
				paramValues.push_back(kwstring());
				state = 4;
				break;
			case '\t':
			case ' ':
				paramValues.push_back(kwstring());
				state = 7;
				break;
			default:
				paramValues.back() += charToUnicode(c);
				break;
			}
			break;
		case 7:
			switch (c)
			{
			case '\r':
			case '\n':
				isRead = false;
				state = 8;
				break;
			case '\t':
			case ' ':
				break;
			case ',':
				state = 4;
				break;
			default:
				isRead = false;
				state = 1;
				break;
			}
			break;
		case 8:
			processParam(paramName, paramValues);
			isRead = false;
			state = 1;
			break;
		default:
			break;
		}
	}
	if ((state == 6) || (state == 7))
		processParam(paramName, paramValues);
}

bool SystemConfig::load()
{
	if (!BootIo::getInstance().readFile(L"config.sys", 0x10000, nullptr, (void**)&m_configData, &m_configSize))
		return false;

	parseFile();
	return true;
}
