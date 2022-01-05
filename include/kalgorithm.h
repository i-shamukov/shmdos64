/*
   kalgorithm.h
   Small algrorithm implementation header for SHM DOS64
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

template<typename InputIterator, typename OutputIterator>
OutputIterator kcopy(InputIterator first, InputIterator last, OutputIterator result)
{
	while (first != last)
	{
		*result = *first;
		++result;
		++first;
	}
	return result;
}

template <typename ForwardIterator, typename T>
void kfill(ForwardIterator first, ForwardIterator last, const T& val)
{
	while (first != last)
	{
		*first = val;
		++first;
	}
}

template <class T>
void kswap(T& a, T& b)
{
	T c(a);
	a = b;
	b = c;
}

template<class InputIterator, class T>
InputIterator kfind(InputIterator first, InputIterator last, const T& val)
{
	while (first != last)
	{
		if (*first == val)
			return first;
		++first;
	}
	return last;
}

template<class ForwardIt, class T>
ForwardIt klower_bound(ForwardIt first, ForwardIt last, const T& value)
{
	size_t count = last - first;
	while (count > 0)
	{
		ForwardIt it = first;
		const size_t step = count / 2;
		it += step;
		if (*it < value)
		{
			first = ++it;
			count -= step + 1;
		}
		else
			count = step;
	}
	return first;
}

template<class BidirIt>
void kreverse(BidirIt first, BidirIt last)
{
	while ((first != last) && (first != --last))
	{
		kswap(*first++, *last);
	}
}

template<typename T>
T kmax(T a, T b)
{
	return a > b ? a : b;
}

template<typename T>
T kmin(T a, T b)
{
	return a < b ? a : b;
}
