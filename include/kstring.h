/*
   kstring.h
   Simple string implementation header for SHM DOS64
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
#include <kvector.h>
#include <kclib.h>

template<typename T>
class kbasic_string
{
public:
	kbasic_string()
	{
	}

	kbasic_string(const kbasic_string<T>& other)
		: m_data(other.m_data)
	{

	}

	kbasic_string(const T* other)
		: m_data(other, &other[kstrlen(other) + 1])
	{

	}

	kbasic_string(kbasic_string<T>&& other)
		: m_data(std::move(other.m_data))
	{

	}

	const T* c_str() const
	{
		static const T emptySymbol = 0;
		return (m_data.empty() ? &emptySymbol : m_data.data());
	}
	
	T* begin() const
	{
		return m_data.begin();
	}
	
	T* end() const
	{
		return (m_data.empty() ? m_data.end() : (m_data.end() - 1));
	}

	kbasic_string& operator=(const kbasic_string<T>& other)
	{
		if (this != &other)
			m_data = other.m_data;
		return *this;
	}

	kbasic_string& operator=(kbasic_string<T>&& other)
	{
		if (this != &other)
			m_data = std::move(other.m_data);
		return *this;
	}

	kbasic_string& operator=(const T* other)
	{
		const size_t len = kstrlen(other) + 1;
		m_data.resize(len);
		kmemcpy(m_data.data(), other, len * sizeof(T));
		return *this;
	}

	size_t size() const
	{
		return (m_data.empty() ? 0 : (m_data.size() - 1));
	}

	bool empty() const
	{
		return m_data.empty();
	}

	void clear()
	{
		m_data.clear();
	}

	kbasic_string& operator+=(const kbasic_string<T>& other)
	{
		if (!other.empty())
		{
			const size_t otherSize = other.size() + 1;
			const size_t curSize = size();
			m_data.resize(curSize + otherSize);
			kmemcpy(&m_data[curSize], other.m_data.data(), otherSize * sizeof(T));
		}
		return *this;
	}

	kbasic_string& operator+=(const T* other)
	{
		const size_t otherSize = kstrlen(other) + 1;
		const size_t curSize = size();
		m_data.resize(curSize + otherSize);
		kmemcpy(&m_data[curSize], other, otherSize * sizeof(T));
		return *this;
	}

	kbasic_string& operator+=(const T symbol)
	{
		if (!m_data.empty())
			m_data.back() = symbol;
		else
			m_data.push_back(symbol);
		m_data.push_back(0);
		return *this;
	}

	bool operator==(const T* other) const
	{
		return (kstrcmp(c_str(), other) == 0);
	}

	bool operator==(const kbasic_string<T>& other) const
	{
		return ((size() == other.size()) && (kstrcmp(c_str(), other.c_str()) == 0));
	}

private:
	kvector<T> m_data;
};

template<typename T>
static kbasic_string<T> operator+(const T* str1, const kbasic_string<T>& str2)
{
	kbasic_string<T> result(str1);
	result += str2;
	return result;
}

typedef kbasic_string<char> kstring;
typedef kbasic_string<wchar_t> kwstring;
