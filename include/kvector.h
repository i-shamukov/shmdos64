/*
   kvector.h
   vector implementation header for SHM DOS64
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
#include <kalgorithm.h>
#include <utility>
#include <kclib.h>

template<typename T>
class kvector
{
public:
	typedef T* iterator;
	typedef const T* const_iterator;

	kvector()
	{
		_init(0);
	}

	kvector(size_t size)
	{
		_init(size);
	}

	kvector(const kvector<T>& other)
	{
		_init(other.size());
		_copy_buf(m_buf, other.m_buf, other.size());
	}

	kvector(const_iterator begin, const_iterator end)
	{
		const size_t size = end - begin;
		_init(size);
		_copy_buf(m_buf, begin, size);
	}

	kvector(kvector<T>&& other)
	{
		_move(other);
	}

	kvector(std::initializer_list<T> lst)
	{
		_init(lst.size());
		_copy_buf(m_buf, lst.begin(), lst.size());
	}

	~kvector()
	{
		_free_all();
	}

	kvector<T>& operator=(const kvector<T>& other)
	{
		if (this != &other)
		{
			if (m_allocSize < other.size())
			{
				_free_all();
				_init(other.size());
			}
			else
			{
				if (m_size > other.m_size)
					_destruct(&m_buf[other.m_size], m_size - other.m_size);
				m_size = other.m_size;
			}
			_copy_buf(m_buf, other.m_buf, other.size());
		}
		return *this;
	}

	kvector<T>& operator=(kvector<T>&& other)
	{
		if (this != &other)
		{
			_free_all();
			_move(other);
		}
		return *this;
	}

	T* data()
	{
		return m_buf;
	}

	T* data() const
	{
		return m_buf;
	}

	size_t size() const
	{
		return m_size;
	}

	void push_back(const T& item)
	{
		if (m_size == m_allocSize)
			reserve(kmax(m_allocSize * 2, m_defaultSize));
		new (&m_buf[m_size++]) T(item);
	}

	template<typename... Args>
	void emplace_back(Args&&... args)
	{
		if (m_size == m_allocSize)
			reserve(kmax(m_allocSize * 2, m_defaultSize));
		new (&m_buf[m_size++]) T(std::forward<Args>(args)...);
	}

	T& operator[](const size_t idx)
	{
		return m_buf[idx];
	}

	const T& operator[](const size_t idx) const
	{
		return m_buf[idx];
	}

	iterator begin() const
	{
		return &m_buf[0];
	}

	iterator end() const
	{
		return &m_buf[m_size];
	}

	bool empty() const
	{
		return (m_size == 0);
	}

	T& front() const
	{
		return m_buf[0];
	}

	T& back() const
	{
		return m_buf[m_size - 1];
	}

	void pop_back()
	{
		_destruct(&m_buf[--m_size], 1);
	}

	void clear()
	{
		_destruct(m_buf, m_size);
		m_size = 0;
	}

	void clear_and_free()
	{
		_free_all();
		_init(0);
	}

	void resize(size_t size)
	{
		reserve(size);
		if (size < m_size)
			_destruct(&m_buf[size], m_size - size);
		else if (size > m_size)
			_construct(&m_buf[m_size], size - m_size);
		m_size = size;
	}

	void reserve(size_t size)
	{
		if (size > m_allocSize)
		{
			auto tmp = _alloc(size);
			if (m_buf != nullptr)
			{
				_move_buf(tmp, m_buf, m_size);
				_free_all();
			}
			m_buf = tmp;
			m_allocSize = size;
		}
	}

	void swap(kvector<T>& other)
	{
		kswap(m_buf, other.m_buf);
		kswap(m_allocSize, other.m_allocSize);
		kswap(m_size, other.m_size);
	}

private:
	T* m_buf;
	size_t m_allocSize;
	size_t m_size;
	static const size_t m_defaultSize = std::is_trivial<T>::value ? 16 : 1;

private:
	T* _alloc(size_t size)
	{
		return static_cast<T*> ((operator new[])(size * sizeof (T)));
	}

	void _free(T* p)
	{
		(operator delete[])(p);
	}

	template<typename DestructType>
	void _destruct(DestructType* p, size_t size,
				   typename std::enable_if<!std::is_trivial<DestructType>::value>::type* = nullptr)
	{
		for (size_t i = 0; i < size; i++)
			p[i].~T();
	}

	template<typename DestructType>
	void _destruct(DestructType*, size_t,
				   typename std::enable_if<std::is_trivial<DestructType>::value>::type* = nullptr)
	{
	}

	template<typename ConstructType>
	void _construct(ConstructType* p, size_t size,
					typename std::enable_if<!std::is_trivial<ConstructType>::value>::type* = nullptr)
	{
		for (size_t i = 0; i < size; ++i)
			new (&p[i]) T;
	}

	template<typename ConstructType>
	void _construct(ConstructType*, size_t,
					typename std::enable_if<std::is_trivial<ConstructType>::value>::type* = nullptr)
	{
	}

	void _free_all()
	{
		if (m_buf != nullptr)
		{
			_destruct(m_buf, m_size);
			_free(m_buf);
		}
	}

	void _init(size_t size)
	{
		m_buf = (size > 0) ? _alloc(size) : nullptr;
		m_allocSize = size;
		m_size = size;
		_construct(m_buf, size);
	}

	void _move(kvector<T>& other)
	{
		m_buf = other.m_buf;
		m_allocSize = other.m_allocSize;
		m_size = other.m_size;
		other._init(0);
	}

	template<typename CopyType>
	void _move_buf(CopyType* to, CopyType* from, size_t size,
				   typename std::enable_if<std::is_trivial<CopyType>::value>::type* = nullptr)
	{
		kmemcpy(to, from, size * sizeof (CopyType));
	}

	template<typename CopyType>
	void _move_buf(CopyType* to, CopyType* from, size_t size,
				   typename std::enable_if<!std::is_trivial<CopyType>::value>::type* = nullptr)
	{
		while (size--)
		{
			new (to) T(std::move(*from));
			to++;
			from++;
		}
	}

	template<typename CopyType>
	void _copy_buf(CopyType* to, const CopyType* from, size_t size,
				   typename std::enable_if<std::is_trivial<CopyType>::value>::type* = nullptr)
	{
		kmemcpy(to, from, size * sizeof (CopyType));
	}

	template<typename CopyType>
	void _copy_buf(CopyType* to, const CopyType* from, size_t size,
				   typename std::enable_if<!std::is_trivial<CopyType>::value>::type* = nullptr)
	{
		copy(from, from + size, to);
	}
};
