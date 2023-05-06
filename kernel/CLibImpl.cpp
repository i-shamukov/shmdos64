/*
   CLibImpl.cpp
   C RTL Implementaion
   SHM DOS64
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

#include <limits>
#include <cstdarg>
#include <kernel_export.h>
#include <kclib.h>
#include <conout.h>

enum SymbolAttibute
{
    SymbolAttibuteUpper = (1 << 0),
    SymbolAttibuteLower = (1 << 1),
    SymbolAttibuteDigit = (1 << 2),
    SymbolAttibuteSpace = (1 << 3),
    SymbolAttibutePunct = (1 << 4),
    SymbolAttibuteControl = (1 << 5),
    SymbolAttibuteBlank = (1 << 6),
    SymbolAttibuteHex = (1 << 7)
};

static const uint8_t g_symbolAttribute[256]=
{
    SymbolAttibuteControl, // 00 (NUL)
    SymbolAttibuteControl, // 01 (SOH)
    SymbolAttibuteControl, // 02 (STX)
    SymbolAttibuteControl, // 03 (ETX)
    SymbolAttibuteControl, // 04 (EOT)
    SymbolAttibuteControl, // 05 (ENQ)
    SymbolAttibuteControl, // 06 (ACK)
    SymbolAttibuteControl, // 07 (BEL)
    SymbolAttibuteControl, // 08 (BS)
    SymbolAttibuteSpace | SymbolAttibuteControl, // 09 (HT)
    SymbolAttibuteSpace | SymbolAttibuteControl, // 0A (LF)
    SymbolAttibuteSpace | SymbolAttibuteControl, // 0B (VT)
    SymbolAttibuteSpace | SymbolAttibuteControl, // 0C (FF)
    SymbolAttibuteSpace | SymbolAttibuteControl, // 0D (CR)
    SymbolAttibuteControl, // 0E (SI)
    SymbolAttibuteControl, // 0F (SO)
    SymbolAttibuteControl, // 10 (DLE)
    SymbolAttibuteControl, // 11 (DC1)
    SymbolAttibuteControl, // 12 (DC2)
    SymbolAttibuteControl, // 13 (DC3)
    SymbolAttibuteControl, // 14 (DC4)
    SymbolAttibuteControl, // 15 (NAK)
    SymbolAttibuteControl, // 16 (SYN)
    SymbolAttibuteControl, // 17 (ETB)
    SymbolAttibuteControl, // 18 (CAN)
    SymbolAttibuteControl, // 19 (EM)
    SymbolAttibuteControl, // 1A (SUB)
    SymbolAttibuteControl, // 1B (ESC)
    SymbolAttibuteControl, // 1C (FS)
    SymbolAttibuteControl, // 1D (GS)
    SymbolAttibuteControl, // 1E (RS)
    SymbolAttibuteControl, // 1F (US)
    SymbolAttibuteSpace | SymbolAttibuteBlank, // 20 SPACE
    SymbolAttibutePunct, // 21 !
    SymbolAttibutePunct, // 22 "
    SymbolAttibutePunct, // 23 #
    SymbolAttibutePunct, // 24 $
    SymbolAttibutePunct, // 25 %
    SymbolAttibutePunct, // 26 &
    SymbolAttibutePunct, // 27 '
    SymbolAttibutePunct, // 28 (
    SymbolAttibutePunct, // 29 )
    SymbolAttibutePunct, // 2A *
    SymbolAttibutePunct, // 2B +
    SymbolAttibutePunct, // 2C ,
    SymbolAttibutePunct, // 2D -
    SymbolAttibutePunct, // 2E .
    SymbolAttibutePunct, // 2F /
    SymbolAttibuteDigit | SymbolAttibuteHex, // 30 0
    SymbolAttibuteDigit | SymbolAttibuteHex, // 31 1
    SymbolAttibuteDigit | SymbolAttibuteHex, // 32 2
    SymbolAttibuteDigit | SymbolAttibuteHex, // 33 3
    SymbolAttibuteDigit | SymbolAttibuteHex, // 34 4
    SymbolAttibuteDigit | SymbolAttibuteHex, // 35 5
    SymbolAttibuteDigit | SymbolAttibuteHex, // 36 6
    SymbolAttibuteDigit | SymbolAttibuteHex, // 37 7
    SymbolAttibuteDigit | SymbolAttibuteHex, // 38 8
    SymbolAttibuteDigit | SymbolAttibuteHex, // 39 9
    SymbolAttibutePunct, // 3A :
    SymbolAttibutePunct, // 3B ;
    SymbolAttibutePunct, // 3C <
    SymbolAttibutePunct, // 3D =
    SymbolAttibutePunct, // 3E >
    SymbolAttibutePunct, // 3F ?
    SymbolAttibutePunct, // 40 @
    SymbolAttibuteUpper | SymbolAttibuteHex, // 41 A
    SymbolAttibuteUpper | SymbolAttibuteHex, // 42 B
    SymbolAttibuteUpper | SymbolAttibuteHex, // 43 C
    SymbolAttibuteUpper | SymbolAttibuteHex, // 44 D
    SymbolAttibuteUpper | SymbolAttibuteHex, // 45 E
    SymbolAttibuteUpper | SymbolAttibuteHex, // 46 F
    SymbolAttibuteUpper, // 47 G
    SymbolAttibuteUpper, // 48 H
    SymbolAttibuteUpper, // 49 I
    SymbolAttibuteUpper, // 4A J
    SymbolAttibuteUpper, // 4B K
    SymbolAttibuteUpper, // 4C L
    SymbolAttibuteUpper, // 4D M
    SymbolAttibuteUpper, // 4E N
    SymbolAttibuteUpper, // 4F O
    SymbolAttibuteUpper, // 50 P
    SymbolAttibuteUpper, // 51 Q
    SymbolAttibuteUpper, // 52 R
    SymbolAttibuteUpper, // 53 S
    SymbolAttibuteUpper, // 54 T
    SymbolAttibuteUpper, // 55 U
    SymbolAttibuteUpper, // 56 V
    SymbolAttibuteUpper, // 57 W
    SymbolAttibuteUpper, // 58 X
    SymbolAttibuteUpper, // 59 Y
    SymbolAttibuteUpper, // 5A Z
    SymbolAttibutePunct, // 5B [
    SymbolAttibutePunct, // 5C  
    SymbolAttibutePunct, // 5D ]
    SymbolAttibutePunct, // 5E ^
    SymbolAttibutePunct, // 5F _
    SymbolAttibutePunct, // 60 `
    SymbolAttibuteLower | SymbolAttibuteHex, // 61 a
    SymbolAttibuteLower | SymbolAttibuteHex, // 62 b
    SymbolAttibuteLower | SymbolAttibuteHex, // 63 c
    SymbolAttibuteLower | SymbolAttibuteHex, // 64 d
    SymbolAttibuteLower | SymbolAttibuteHex, // 65 e
    SymbolAttibuteLower | SymbolAttibuteHex, // 66 f
    SymbolAttibuteLower, // 67 g
    SymbolAttibuteLower, // 68 h
    SymbolAttibuteLower, // 69 i
    SymbolAttibuteLower, // 6A j
    SymbolAttibuteLower, // 6B k
    SymbolAttibuteLower, // 6C l
    SymbolAttibuteLower, // 6D m
    SymbolAttibuteLower, // 6E n
    SymbolAttibuteLower, // 6F o
    SymbolAttibuteLower, // 70 p
    SymbolAttibuteLower, // 71 q
    SymbolAttibuteLower, // 72 r
    SymbolAttibuteLower, // 73 s
    SymbolAttibuteLower, // 74 t
    SymbolAttibuteLower, // 75 u
    SymbolAttibuteLower, // 76 v
    SymbolAttibuteLower, // 77 w
    SymbolAttibuteLower, // 78 x
    SymbolAttibuteLower, // 79 y
    SymbolAttibuteLower, // 7A z
    SymbolAttibutePunct, // 7B {
    SymbolAttibutePunct, // 7C |
    SymbolAttibutePunct, // 7D }
    SymbolAttibutePunct, // 7E ~
    SymbolAttibuteControl, // 7F (DEL)
	SymbolAttibuteUpper, // 80
	SymbolAttibuteUpper, // 81
	SymbolAttibuteUpper, // 82
	SymbolAttibuteUpper, // 83
	SymbolAttibuteUpper, // 84 
	SymbolAttibuteUpper, // 85
	SymbolAttibuteUpper, // 86
	SymbolAttibuteUpper, // 87
	SymbolAttibuteUpper, // 88
	SymbolAttibuteUpper, // 89
	SymbolAttibuteUpper, // 8A
	SymbolAttibuteUpper, // 8B
	SymbolAttibuteUpper, // 8C
	SymbolAttibuteUpper, // 8D
	SymbolAttibuteUpper, // 8E
	SymbolAttibuteUpper, // 8F
	SymbolAttibuteUpper, // 90
	SymbolAttibuteUpper, // 91
	SymbolAttibuteUpper, // 92
	SymbolAttibuteUpper, // 93
	SymbolAttibuteUpper, // 94
	SymbolAttibuteUpper, // 95
	SymbolAttibuteUpper, // 96
	SymbolAttibuteUpper, // 97
	SymbolAttibuteUpper, // 98
	SymbolAttibuteUpper, // 99
	SymbolAttibuteUpper, // 9A
	SymbolAttibuteUpper, // 9B
	SymbolAttibuteUpper, // 9C
	SymbolAttibuteUpper, // 9D
	SymbolAttibuteUpper, // 9E
	SymbolAttibuteUpper, // 9F
	SymbolAttibuteLower, // A0
	SymbolAttibuteLower, // A1
	SymbolAttibuteLower, // A2
	SymbolAttibuteLower, // A3
	SymbolAttibuteLower, // A4
	SymbolAttibuteLower, // A5
	SymbolAttibuteLower, // A6
	SymbolAttibuteLower, // A7
	SymbolAttibuteLower, // A8
	SymbolAttibuteLower, // A9
	SymbolAttibuteLower, // AA
	SymbolAttibuteLower, // AB
	SymbolAttibuteLower, // AC
	SymbolAttibuteLower, // AD
	SymbolAttibuteLower, // AE
	SymbolAttibuteLower, // AF
	0, // B0
	0, // B1
	0, // B2
	0, // B3
	0, // B4
	0, // B5
	0, // B6
	0, // B7
	0, // B8
	0, // B9
	0, // BA
	0, // BB
	0, // BC
	0, // BD
	0, // BE
	0, // BF
	0, // C0
	0, // C1
	0, // C2
	0, // C3
	0, // C4
	0, // C5
	0, // C6
	0, // C7
	0, // C8
	0, // C9
	0, // CA
	0, // CB
	0, // CC
	0, // CD
	0, // CE
	0, // CF
	0, // D0
	0, // D1
	0, // D2
	0, // D3
	0, // D4
	0, // D5
	0, // D6
	0, // D7
	0, // D8
	0, // D9
	0, // DA
	0, // DB
	0, // DC
	0, // DD
	0, // DE
	0, // DF
	SymbolAttibuteLower, // E0
	SymbolAttibuteLower, // E1
	SymbolAttibuteLower, // E2
	SymbolAttibuteLower, // E3
	SymbolAttibuteLower, // E4
	SymbolAttibuteLower, // E5
	SymbolAttibuteLower, // E6
	SymbolAttibuteLower, // E7
	SymbolAttibuteLower, // E8
	SymbolAttibuteLower, // E9
	SymbolAttibuteLower, // EA
	SymbolAttibuteLower, // EB
	SymbolAttibuteLower, // EC
	SymbolAttibuteLower, // ED
	SymbolAttibuteLower, // EE
	SymbolAttibuteLower, // EF
	SymbolAttibuteUpper, // F0
	SymbolAttibuteLower, // F1
	SymbolAttibuteUpper, // F2
	SymbolAttibuteLower, // F3
	SymbolAttibuteUpper, // F4
	SymbolAttibuteLower, // F5
	SymbolAttibuteUpper, // F6
	SymbolAttibuteLower, // F7
	SymbolAttibutePunct, // F8
	SymbolAttibutePunct, // F9
	SymbolAttibutePunct, // FA
	SymbolAttibutePunct, // FB
	SymbolAttibutePunct, // FC
	SymbolAttibutePunct, // FD
	SymbolAttibutePunct, // FE
	0, // FF
};

static inline bool checkSymbol(char c, uint8_t attr)
{
    return ((g_symbolAttribute[static_cast<unsigned char>(c)] & attr) != 0);
}


unsigned long kstrtoul(const char *start, const char **end, int radix)
{
	unsigned long val = 0, digit, tmp;
	char c;
	while(*start == '0')
		start++;
	switch (radix)
	{
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
		case 7:
		case 8:
		case 9:
		case 10:
		{
			while( (c = *start) != '\0' )
			{
				if (!checkSymbol(c, SymbolAttibuteDigit))
					break;

				digit = c - '0';
				if (digit >= (unsigned long)radix)
					break;

				tmp = val * radix;
				if (tmp < val)
				{
					val = std::numeric_limits<unsigned long>::max();
					break;
				}

				tmp += digit;
				val = tmp;
				start++;
			}
			if (end != nullptr)
				*end = start;
			return val;
		}
		case 16:
		{
			const int maxIt = sizeof(unsigned long) * 2;
			for (int i = 0; i < maxIt; i++)
			{
				c = *start++;
				if (checkSymbol(c, SymbolAttibuteDigit))
				{
					val <<= 4;
					val |= (c - '0');
				}
				else if (checkSymbol(c, SymbolAttibuteHex))
				{
					val <<= 4;
					if (checkSymbol(c, SymbolAttibuteUpper))
						val |= (c - 'A' + 10);
					else
						val |= (c - 'a' + 10);
				}
				else
				{
					if (end != nullptr)
						*end = start;
					return val;
				}
			}
			if (end != nullptr)
				*end = start;
			if (checkSymbol(*start, SymbolAttibuteHex))
				return std::numeric_limits<unsigned long>::max();

			return val;
		}
		default:
#ifdef CLIB_IMPL_DEBUG
			println("strtoul unsupp_radix ", radix);
#endif
			break;
	}
	return 0;
}

int kvprintf(const char* format, va_list arg)
{
	int symbolCounter = 0;
	int state = 0;
	char argBuf[32];
	const char *pArg;
	bool left = false;
    bool sign = false; 
    bool space = false; 
    bool number = false; 
    bool numPositive = true;
	char fill = ' ';
	unsigned int width = 0;
    unsigned int precision = 0;
	size_t argLen;
	for (const char* p = format; *p != '\0'; )
	{
        const char c = *p;
		switch (state)
		{
			case 0:
				if (c != '%')
				{
					print(c);
					symbolCounter++;
				}
				else
					state = 1;
				p++;
				break;

			case 1:
				switch (c)
				{
					case '-':
						left = true;
						p++;
						continue;
					case '+':
						sign = true;
						p++;
						continue;
					case ' ':
						space = true;
						p++;
						continue;
					case '0':
						fill = '0';
						p++;
						continue;
					default:
						break;
				}
				if (checkSymbol(c, SymbolAttibuteDigit))
				{
					width = kstrtoul(p, &p, 10);
					state = 2;
					continue;
				}
				if (c == '*')
				{
					width = va_arg(arg, unsigned int);
					p++;
					state = 2;
					continue;
				}
                 [[fallthrough]];
			case 2:
				if (c == '.')
				{
					state = 3;
					p++;
					continue;
				}
                [[fallthrough]];
			case 4:
				switch (c)
				{
					case 's':
						pArg = va_arg(arg, const char*);
						number = false;
						break;

					case 'u':
                    {
						unsigned int uintArg = va_arg(arg, unsigned int);
						pArg = argBuf;
                        integerToDecStr(uintArg, argBuf);
						number = true;
						numPositive = true;
						break;
                    }

					case 'd':
                    {
						int intArg = va_arg(arg, int);
						pArg = argBuf;
						if (intArg >= 0)
						{	
							numPositive = true;
                            integerToDecStr(intArg, argBuf);
						}
						else
						{
							numPositive = false;
							sign = true;
                            integerToDecStr(-intArg, argBuf);
						}
						number = true;
						break;
                    }

					case 'c':
                    {
						argBuf[0] = va_arg(arg, int);
                        argBuf[1] = '\0';
                        pArg = argBuf;
						precision = 1;
						number = false;
						break;
                    }

					case 'X':
                    {
						unsigned int uintArg = va_arg(arg, unsigned int);
						pArg = argBuf;
						numPositive = true;
						unsignedIntegerToHexStr(uintArg, argBuf, 16);
						number = true;
						break;
                    }

					default:
#ifdef CLIB_IMPL_DEBUG
						println("Unknown printf format: ", c)
#endif
						state = 0;
						break;
				}
				if (state == 0)
				{
					p++;
					break;
				}
				if (number)
				{
					size_t widthLen;
					if (precision != 0)
						argLen = kstrlen(pArg);
					else
						argLen = 0;
					if (width != 0)
					{
						if (argLen == 0)
							argLen = kstrlen(pArg);
						widthLen = argLen;
						if (widthLen < precision)
							widthLen = precision;
						if (sign)
							widthLen++;
						if (width > widthLen)
							widthLen = width - widthLen;
						else
						{
							widthLen = 0;
							if (space)
								print(' ');
						}
						if (left)
						{
							while (widthLen > 0)
							{
								print(fill);
								widthLen--;
							}
						}
					}
					else
						widthLen = 0;
					if (sign)
						print(numPositive ? '+' : '-');
					if (argLen < precision)
					{
						size_t add = precision - argLen;
						while (add-- > 0)
							print('0');
					}
					print(pArg);
					while (widthLen-- > 0)
						print(fill);
				}
				else
				{
					size_t widthLen;
					if (width != 0)
					{
						widthLen = kstrlen(pArg);
						if (precision < widthLen)
							widthLen = precision;
						if (width > widthLen)
						{
							widthLen = width - widthLen;
							if (left)
							{
								while (widthLen > 0)
								{
									print(fill);
									widthLen--;
								}
							}
						}
						else
							widthLen = 0;
					}
					else
						widthLen = 0;
					if (precision == 0)
						print(pArg);
					else
					{
						for (unsigned int i = 0; i < precision; i++)
							if (pArg[i] == '\0')
								break;
							else
								print(pArg[i]);
					}
					while (widthLen-- > 0)
						print(fill);
				}
				p++;
				state = 0;
				break;

			case 3:
				precision = kstrtoul(p, &p, 10);
				state = 4;
				continue;

			default:
				break;
		}		
	}
	return symbolCounter;
}

extern "C"
{
    KERNEL_SHARED int memcmp(const void* lhs, const void* rhs, size_t size)
    {
        return kstrncmp<uint8_t>(static_cast<const uint8_t*>(lhs), static_cast<const uint8_t*>(rhs), size);
    }

    KERNEL_SHARED void* memcpy(void* dst, const void* src, size_t size)
    {
        return kmemcpy(dst, src, size);
    }

    KERNEL_SHARED size_t strlen(const char* str)
    {
        return kstrlen(str);
    }

    KERNEL_SHARED char* strcpy(char* dest, const char* src )
    {
        return kstrcpy(dest, src);
    }

    KERNEL_SHARED char* strncpy(char* dest, const char* src, size_t count)
    {
        return kstrncpy(dest, src, count);
    }

	KERNEL_SHARED int __imp_isprint(int ch)
    {
        return checkSymbol(ch, SymbolAttibuteControl) ? 0 : 1;
    }

	KERNEL_SHARED int isprint(int ch)
    {
        return __imp_isprint(ch);
    }

	KERNEL_SHARED int strcmp(const char* str1, const char* str2)
    {
        return kstrcmp(str1, str2);
    }

	KERNEL_SHARED char* strcat(char* destination, const char* source)
    {
        return kstrcat(destination, source);
    }

	KERNEL_SHARED unsigned long strtoul(const char *start, const char **end, int radix)
	{
		return kstrtoul(start, end, radix);
	}

	KERNEL_SHARED int __imp_isxdigit(int ch)
    {
        return checkSymbol(ch, SymbolAttibuteHex) ? 1 : 0;
    }

	KERNEL_SHARED int isxdigit(int ch)
    {
        return __imp_isxdigit(ch);
    }

	KERNEL_SHARED int __imp_toupper(int ch)
    {
        return ktoupper<char>(ch);
    }

	KERNEL_SHARED int toupper(int ch)
    {
        return __imp_toupper(ch);
    }

	KERNEL_SHARED int __imp_tolower(int ch)
    {
        return ktolower<char>(ch);
    }

	KERNEL_SHARED int tolower(int ch)
    {
        return __imp_tolower(ch);
    }

	KERNEL_SHARED int __imp_isspace(int ch)
    {
        return checkSymbol(ch, SymbolAttibuteSpace) ? 1 : 0;
    }

	KERNEL_SHARED int isspace(int ch)
    {
        return __imp_isspace(ch);
    }
}