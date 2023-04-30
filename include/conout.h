/*
   conout.h
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

#include <type_traits>
#include <kalgorithm.h>
#include <kstring.h>

extern void print(const char* str);
extern void print(char symbol);

extern void print(const wchar_t* str);
extern void print(wchar_t symbol);

template <typename T>
void integerToDecStr(T num, char* str, typename std::enable_if<std::is_unsigned<T>::value>::type* = nullptr)
{
	char* ptr = str;
	for (; num > 0; num /= 10)
		*ptr++ = '0' + (num % 10);
	if (ptr == str)
		*ptr++ = '0';
	else
		kreverse(str, ptr);
	*ptr++ = '\0';
}

static inline void print(char* str)
{
	print(static_cast<const char*> (str));
}

static inline void print(const kstring& str)
{
	print(str.c_str());
}

static inline void print(const kwstring& str)
{
	print(str.c_str());
}

template <typename T>
static void integerToDecStr(T num, char* str, typename std::enable_if<std::is_signed<T>::value>::type* = nullptr)
{
	if (num < 0)
	{
		num = -num;
		*str++ = '-';
	}
	integerToDecStr(static_cast<typename std::make_unsigned<T>::type> (num), str);
}

template<typename T>
static void print(T num,
				  typename std::enable_if<std::is_integral<T>::value && !std::is_floating_point<T>::value>::type* = nullptr)
{
	char str[64];
	integerToDecStr(num, str);
	print(str);
}

template<typename T>
struct hex
{
	T m_value;
	bool m_skupNulls;

	hex(T value, bool skupNulls = true)
		: m_value(value)
		, m_skupNulls(skupNulls)
	{
	}
};

template<typename T>
static void unsignedIntegerToHexStr(T value, char *str, bool skipNulls)
{
	static_assert(std::is_unsigned<T>::value, "Signed value");
	static const char *htbl = "0123456789ABCDEF";
	int i = sizeof (T) * 8 - 4;
	if (skipNulls)
	{
		for (; i > 0; i -= 4)
			if (((value >> i) & 0xF) != 0)
				break;
	}
	for (; i > 0; i -= 4)
		*str++ = htbl[(value >> i) & 0xF];
	*str++ = htbl[value & 0xF];
	*str++ = '\0';
}

template<typename T>
static void print(const hex<T>& hexNum)
{
	char str[64];
	unsignedIntegerToHexStr(static_cast<typename std::make_unsigned<T>::type> (hexNum.m_value), str, hexNum.m_skupNulls);
	print(str);
}

template<typename T>
static inline void print(T ptr, typename std::enable_if<std::is_pointer<T>::value>::type* = nullptr)
{
	print(hex(reinterpret_cast<uintptr_t> (ptr), false));
}

template<typename T, typename... Args>
static void print(T t, Args... args)
{
	print(t);
	print(args...);
}

template<typename... Args>
static void println(Args... args)
{
	print(args...);
	print('\n');
}
