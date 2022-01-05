/*
   conout.cpp
   Simple console output for bootloader
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

#include <conout.h>
#include <klocale.h>
#include "UefiVideo.h"

void print(const char *str)
{
	auto& video = UefiVideo::getInstance();
	while (*str != '\0')
		video.putc(*str++);
}

void print(char symbol)
{
	UefiVideo::getInstance().putc(symbol);
}

void print(const wchar_t *str)
{
	auto& video = UefiVideo::getInstance();
	while (*str != '\0')
		video.putc(unicodeToChar(*str++));
}

void print(wchar_t symbol)
{
	UefiVideo::getInstance().putc(unicodeToChar(symbol));
}
