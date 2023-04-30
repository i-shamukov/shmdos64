/*
   klocale.h
   Shared header for SHM DOS64
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
#include <common_types.h>

static inline wchar_t oem866ToUnicode(char ch)
{
	wchar_t c = ch;
	if(c > 0x80)
	{
		if( (c >= 0x80) && (c <= 0x9F) )
			c = c - 0x80 + L'А';
		else if( (c >= 0xA0) && (c <= 0xAF) )
			c = c - 0xA0 + L'а';
		else if( (c >= 0xE0) && (c <= 0xEF) )
			c = c - 0xE0 + L'р';
		else if(c == 0xF0)
			c = L'Ё';
		else if(c == 0xF1)
			c = L'ё';
		else
			c = L'?';	
	}
	return (wchar_t)ch;
}

static inline char unicodeToOem866(wchar_t c)
{
	if(c > 0x80)
	{
		if( (c >= L'а') && (c <= L'п') )
			c = c - L'а' + 0xA0;
		else if( (c >= L'р') && (c <= L'я') )
			c = c - L'р' + 0xE0;
		else if( (c >= L'А') && (c <= L'Я') )
			c = c - L'А' + 0x80;
		else if(c == L'Ё')
			c = 0xF0;
		else if(c == L'ё')
			c = 0xF1;
		else
			c = L'?';
	}
	return (char)c;
}

static inline wchar_t charToUnicode(char c)
{
    return oem866ToUnicode(c);
}

static inline char unicodeToChar(wchar_t c)
{
    return unicodeToOem866(c);
}

static inline void charStrToUnicode(const char *str, wchar_t *outStr, size_t maxSize)
{
    if (maxSize == 0)
        return;

    maxSize--;
    while (*str != '\0')
    {
        if (maxSize-- == 0)
            break;
        *outStr++ = charToUnicode(*str++);
    }
    *outStr = L'\0';
}

static inline void unicodeToCharStr(const wchar_t *str, char *outStr, size_t maxSize)
{
    if (maxSize == 0)
        return;

    maxSize--;
    while (*str != L'\0')
    {
        if (maxSize-- == 0)
            break;
        *outStr++ = unicodeToChar(*str++);
    }
    *outStr = '\0';
}
