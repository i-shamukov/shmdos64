/*
   kclib.h
   CRT implementation header for SHM DOS64
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

#include <common_types.h>

extern char* kitoa(int value, char *str, int base);
extern char* kuitoa(unsigned value, char *str, int base);

static inline void* kmemcpy(void* dst, const void* src, size_t size)
{
    uint8_t* pb_dst = static_cast<uint8_t*> (dst);
    const uint8_t* pb_src = static_cast<const uint8_t*> (src);
    uintptr_t rest = (uintptr_t) dst & (sizeof (uintptr_t) - 1);
    if (rest != 0)
    {
        if (rest >= size)
        {
            while (size-- > 0)
                *pb_dst++ = *pb_src++;
            return dst;
        }
        else
        {
            size -= rest;
            while (rest-- > 0)
                *pb_dst++ = *pb_src++;
        }
    }
    auto p_dst = reinterpret_cast<uintptr_t*> (pb_dst);
    auto p_end = p_dst + (size / sizeof (*p_dst));
    auto p_src = reinterpret_cast<const uintptr_t*> (pb_src);
    while (p_dst != p_end)
        *p_dst++ = *p_src++;
    rest = size & (sizeof (*p_dst) - 1);
    pb_dst = reinterpret_cast<uint8_t*> (p_dst);
    pb_src = reinterpret_cast<const uint8_t*> (p_src);
    while (rest-- > 0)
        *pb_dst++ = *pb_src++;
    return dst;
}

static inline void* kmemset(void* ptr, int value, size_t size)
{
    uint8_t* pb_dst = static_cast<uint8_t*> (ptr);
    uintptr_t rest = (uintptr_t) ptr & (sizeof (uintptr_t) - 1);
    if (rest != 0)
    {
        if (rest >= size)
        {
            while (size-- > 0)
                *pb_dst++ = value;
            return ptr;
        }
        else
        {
            size -= rest;
            while (rest-- > 0)
                *pb_dst++ = value;
        }
    }
    auto p_dst = reinterpret_cast<uintptr_t*> (pb_dst);
    auto p_end = p_dst + (size / sizeof (*p_dst));
    if (p_dst != p_end)
    {
        uintptr_t pattern;
        auto pbPattern = reinterpret_cast<uint8_t*> (&pattern);
        for (size_t i = 0; i < sizeof (pattern); i++)
            *pbPattern++ = value;
        while (p_dst != p_end)
            *p_dst++ = pattern;
    }
    rest = size & (sizeof (*p_dst) - 1);
    pb_dst = reinterpret_cast<uint8_t*> (p_dst);
    while (rest-- > 0)
        *pb_dst++ = value;
    return ptr;
}

template< typename T >
static inline int kstrncmp(const T* str1, const T* str2, size_t len)
{
    while (len-- > 0)
    {
        if (*str1 != *str2)
            return ( (*str1 < *str2) ? -1 : 1);
        str1++;
        str2++;
    }
    return 0;
}

static inline int kmemcmp(const void* p1, const void* p2, size_t len)
{
    return kstrncmp<uint8_t>(static_cast<const uint8_t*> (p1), static_cast<const uint8_t*> (p2), len);
}

template< typename T >
static inline int kstrcmp(const T* str1, const T* str2)
{
    while (*str1 == *str2)
    {
        if (*str1 == '\0')
            return 0;
        str1++;
        str2++;
    }
    return ( (*str1 < *str2) ? -1 : 1);
}

template< typename T >
static T ktoupper(T character)
{
    if (character >= 'a' && character <= 'z')
        return character - 0x20;
    else
        return character;
}

template< typename T >
static inline int kstricmp(const T* str1, const T* str2)
{
    while (ktoupper(*str1) == ktoupper(*str2))
    {
        if (*str1 == '\0')
            return 0;
        str1++;
        str2++;
    }
    return ( (*str1 < *str2) ? -1 : 1);
}

template< typename T >
size_t kstrlen(const T* str)
{
    auto p = str;
    for (p = str; *p; p++);
    return p - str;
}

template< typename T >
T* kstrcpy(T* dst, const T* src)
{
    T* p_dst = dst;
    do
    {
        *p_dst++ = *src;
    } while (*src++);
    return dst;
}

template< typename T >
T* kstrcat(T* str1, const T* str2)
{
    kstrcpy<T>(str1 + kstrlen<T>(str1), str2);
    return str1;
}

template< typename T >
T* kstrncpy(T *dst, const T *src, size_t len)
{
    T* p_dst = dst;
    if (len == 0)
        return dst;
    do
    {
        *p_dst++ = *src++;
        if (--len == 0)
            return dst;
    } while (*src);
    while (len-- > 0)
        *p_dst++ = 0;
    return dst;
}

template< class T, class C >
static inline uintptr_t getObjectOffset(T C::*field)
{
    C tmp;
    return reinterpret_cast<char *> (&(tmp .* field)) - reinterpret_cast<char *> (&tmp);
}
