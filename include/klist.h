/*
   klist.h
   List implementation header for SHM DOS64
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
#include <iterator>
#include <kvector.h>

template<typename T>
class klist
{
	struct _node_item
	{
		T item;
		_node_item* next;
		_node_item* prev;
	};

public:
	class iterator: public std::iterator<std::forward_iterator_tag, T>
	{
	public:
		const bool operator!=(const iterator &it) const
		{
			return (_pitem != it._pitem);
		}

		const bool operator==(const iterator &it) const
		{
			return (_pitem == it._pitem);
		}

		T& operator*() const
		{
			return _pitem->item;
		}

		T* operator->()
		{
			return &_pitem->item;
		}

		const iterator& operator++()
		{
			_pitem = _pitem->next;
			return *this;
		}

	private:
		_node_item *_pitem;
		friend class klist;
	};

public:
	klist()
	{
		_init(m_default_base_size, m_default_max_size);
	}

	~klist()
	{
		_free_all();
	}

	iterator& push_back(const T& item)
	{
		_node_item* pitem = _alloc_item();
		new (&pitem->item) T(item);
		_append_node(pitem);
		return m_end_item;
	}

	template<typename... Args>
	iterator& emplace_back(Args&&... args)
	{
		_node_item* pitem = _alloc_item();
		new (&pitem->item) T(std::forward<Args>(args)...);
		_append_node(pitem);
		return m_end_item;
	}

	const iterator& begin() const
	{
		return m_begin_item;
	}

	iterator& begin()
	{
		return m_begin_item;
	}

	const iterator& end() const
	{
		return m_end_item_ret;
	}

	bool empty() const
	{
		return (begin() == end());
	}

	iterator erase(iterator &it)
	{
		(*it).~T();
		_node_item *cur = it._pitem;
		_node_item *next = cur->next;
		cur->next = m_free_list;
		m_free_list = cur;
		if (m_end_item._pitem == cur)
			m_end_item._pitem = cur->prev;
		if (it._pitem == m_begin_item._pitem)
		{
			m_begin_item._pitem = next;
			return m_begin_item;
		}
		else
		{
			_node_item *prev = cur->prev;
			prev->next = next;
			if (next != nullptr)
				next->prev = prev;
			iterator next_it;
			next_it._pitem = next;
			return next_it;
		}
	}

	T& front()
	{
		return *begin();
	}

	void pop_front()
	{
		_node_item* cur = m_begin_item._pitem;
		cur->item.~T();
		_node_item* next = cur->next;
		cur->next = m_free_list;
		m_free_list = cur;
		if (m_end_item._pitem == cur)
			m_end_item._pitem = nullptr;
		m_begin_item._pitem = next;
	}

	void clear()
	{
		_free_all();
		m_alloc_areas.clear();
		_init(m_default_base_size, m_default_max_size);
	}

	void clear_no_free()
	{
		for (auto it = begin(); it != end(); it = erase(it));
	}

	bool remove_first(const T& item)
	{
		for (auto it = begin(); it != end();)
		{
			if (*it == item)
			{
				erase(it);
				return true;
			}

			++it;
		}
		return false;
	}

	void swap(klist<T>& other)
	{
		m_alloc_areas.swap(other.m_alloc_areas);
		kswap(m_free_list, other.m_free_list);
		kswap(m_begin_item, other.m_begin_item);
		kswap(m_end_item, other.m_end_item);
		kswap(m_end_item_ret, other.m_end_item_ret);
		kswap(m_cur_ar_size, other.m_cur_ar_size);
		kswap(m_max_size, other.m_max_size);
	}

private:
	void _add_free_region(size_t ar_size)
	{
		_node_item* ar = static_cast<_node_item*> ((operator new[])(m_cur_ar_size * sizeof (_node_item)));
		_node_item* it;
		const _node_item* end = ar + ar_size - 1;
		for (it = ar; it < end;)
		{
			_node_item* tmp = it++;
			tmp->next = it;
		}
		it->next = nullptr;
		m_free_list = ar;
		m_alloc_areas.push_back(ar);
	}

	void _init(size_t base_size, size_t max_size)
	{
		m_cur_ar_size = base_size;
		m_max_size = max_size;
		m_begin_item._pitem = nullptr;
		m_end_item._pitem = nullptr;
		m_end_item_ret._pitem = nullptr;
	}

	_node_item* _alloc_item()
	{
		if (m_free_list == nullptr)
		{
			_add_free_region(m_cur_ar_size);
			if (m_cur_ar_size < m_max_size)
			{
				m_cur_ar_size *= 2;
				if (m_cur_ar_size > m_max_size)
					m_cur_ar_size = m_max_size;
			}
		}
		_node_item* item = m_free_list;
		m_free_list = item->next;
		return item;
	}

	template<typename DestructType>
	void _destruct_all(typename std::enable_if<!std::is_trivial<DestructType>::value>::type* = nullptr)
	{
		for (auto it = begin(); it != end(); ++it)
			(*it).~T();
	}

	template<typename DestructType>
	void _destruct_all(typename std::enable_if<std::is_trivial<DestructType>::value>::type* = nullptr)
	{
	}

	void _free_all()
	{
		_destruct_all<T>();
		for (_node_item* area : m_alloc_areas)
			(operator delete[])(area);
	}

	void _append_node(_node_item* pitem)
	{
		if (m_end_item._pitem == nullptr)
		{
			pitem->prev = nullptr;
			pitem->next = nullptr;
			m_end_item._pitem = pitem;
			m_begin_item._pitem = pitem;
		}
		else
		{
			pitem->next = nullptr;
			pitem->prev = m_end_item._pitem;
			m_end_item._pitem->next = pitem;
			m_end_item._pitem = pitem;
		}
	}

private:
	static const size_t m_default_max_size = 0x100000;
	static const size_t m_default_base_size = 0x10;
	kvector<_node_item*> m_alloc_areas;
	_node_item* m_free_list = nullptr;
	iterator m_begin_item;
	iterator m_end_item;
	iterator m_end_item_ret;
	size_t m_cur_ar_size;
	size_t m_max_size;
};
