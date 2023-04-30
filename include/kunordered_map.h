/*
   kunordered_map.h
   unordered_map implementation header for SHM DOS64
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
#include <initializer_list>
#include <kvector.h>
#include <kstring.h>
#include <ContainerAllocator.h>

template<typename T>
struct khash;

#define KUNOREDED_MAP_INT_HASH_DEF(type)\
template<>\
struct khash<type>\
{\
	size_t operator()(type value) const\
	{\
		return static_cast<size_t>(value);\
	}\
};

KUNOREDED_MAP_INT_HASH_DEF(char);
KUNOREDED_MAP_INT_HASH_DEF(unsigned char);
KUNOREDED_MAP_INT_HASH_DEF(short);
KUNOREDED_MAP_INT_HASH_DEF(unsigned short);
KUNOREDED_MAP_INT_HASH_DEF(int);
KUNOREDED_MAP_INT_HASH_DEF(unsigned int);
KUNOREDED_MAP_INT_HASH_DEF(long);
KUNOREDED_MAP_INT_HASH_DEF(unsigned long);
KUNOREDED_MAP_INT_HASH_DEF(long long);
KUNOREDED_MAP_INT_HASH_DEF(unsigned long long);
KUNOREDED_MAP_INT_HASH_DEF(wchar_t);

template<>
struct khash<kstring>
{
	size_t operator()(const kstring& str) const
	{
		static_assert(sizeof(size_t) == sizeof(uint64_t));
		size_t h(525201411107845655ull);
		for (const char c : str) {
			h ^= c;
			h *= 0x5bd1e9955bd1e995;
			h ^= h >> 47;
		}
		return h;
	}
};

template<typename Key, typename T>
class kunordered_map
{
public:
	typedef std::pair<const Key, T> value_type;

private:
	struct ListData
	{
		typename ContainerAllocator<value_type, ListData>::Node* m_prev;
		typename ContainerAllocator<value_type, ListData>::Node* m_next;
	};
	typedef typename ContainerAllocator<value_type, ListData>::Node Node;


public:
	class iterator
	{
	public:
		iterator() = default;
		iterator& operator++()
		{
			m_node = m_node->m_ext.m_next;
			return *this;
		}

		bool operator==(const iterator& oth) const
		{
			return m_node == oth.m_node;
		}

		bool operator!=(const iterator& oth) const
		{
			return m_node != oth.m_node;
		}

		value_type* operator->()
		{
			return &m_node->m_data;
		}

		value_type& operator*()
		{
			return m_node->m_data;
		}

	private:
		iterator(Node* node)
			: m_node(node)
		{
		
		}

	private:
		Node* m_node;
		friend class kunordered_map;
	};

public:
	kunordered_map()
		: m_table(m_tableSize)
	{
		tableInit(m_table.data(), m_tableSize);
	}

	kunordered_map(std::initializer_list<value_type> lst)
		: m_tableSize((lst.size() + 1) * 2)
		, m_rehashSize(m_tableSize / 2)
		, m_table(m_tableSize)
	{
		tableInit(m_table.data(), m_tableSize);
		for (auto it = lst.begin(); it != lst.end(); ++it)
			emplace(*it);
	}

	iterator begin() const
	{
		return iterator{ m_begin };
	}

	iterator end() const
	{
		return iterator{ nullptr };
	}

	size_t size() const
	{
		return m_allocator.size();
	}

	template<typename... Args>
	std::pair<iterator, bool> emplace(Args&&... args)
	{
		value_type value(std::forward<Args>(args)...);
		const size_t hash = m_hash(value.first) % m_tableSize;
		Node* curNode = m_table[hash];
		while (curNode != nullptr)
		{
			if (curNode->m_data.first == value.first)
			{
				curNode->m_data.second.~T();
				new (&curNode->m_data.second) T(std::move(value.second));
				return std::make_pair(iterator{ curNode }, false);
			}

			curNode = curNode->m_next;
		}
		if (m_allocator.size() >= m_rehashSize)
		{
			m_rehashSize = m_tableSize;
			m_tableSize *= 2;
			rehash();
		}
		Node* newNode = m_allocator.add();
		new (&newNode->m_data) value_type(std::move(value));
		newNode->m_next = m_table[hash];
		m_table[hash] = newNode;
		newNode->m_ext.m_next = m_begin;
		newNode->m_ext.m_prev = nullptr;
		if (m_begin != nullptr)
			m_begin->m_ext.m_prev = newNode;
		m_begin = newNode;
		return std::make_pair(iterator{ newNode }, true);
	}

	iterator find(const Key& key) const
	{
		for (Node* node = m_table[m_hash(key) % m_tableSize]; node != nullptr; node = node->m_next)
		{
			if (node->m_data.first == key)
				return iterator{ node };
		}
		return end();
	}

	iterator erase(iterator it)
	{
		it.m_node->m_data.~value_type();
		const iterator ret(it.m_node->m_ext.m_next);
		removeFromExtList(it.m_node);
		Node*& firstNode = m_table[m_hash(it.m_node->m_data.first) % m_tableSize];
		if (firstNode == it.m_node)
		{
			firstNode = nullptr;
		}
		else
		{
			Node* oldNode = firstNode;
			for (Node* node = oldNode->m_next; node != nullptr; node = node->m_next)
			{
				if (node == it.m_node)
				{
					oldNode->m_next = node->m_next;
					break;
				}

				oldNode = node;
			}
		}
		m_allocator.remove(it.m_node);
		return ret;
	}

	int erase(const Key& key)
	{
		Node*& firstNode = m_table[m_hash(key) % m_tableSize];
		Node* oldNode = nullptr;
		for (Node* node = firstNode; node != nullptr; node = node->m_next)
		{
			if (node->m_data.first == key)
			{
				node->m_data.~value_type();
				removeFromExtList(node);
				if (oldNode == nullptr)
					firstNode = nullptr;
				else
					oldNode->m_next = node->m_next;
				m_allocator.remove(node);
				return 1;
			}

			oldNode = node;
		}
		return 0;
	}

private:
	void tableInit(Node** p, size_t size)
	{
		kmemset(p, 0, size * sizeof(*p));
	}

	void rehash()
	{
		kvector<Node*> table(m_tableSize);
		tableInit(table.data(), m_tableSize);
		for (Node* node = m_begin; node != nullptr; node = node->m_next)
			table[m_hash(node->m_data.first) % m_tableSize] = node;
		m_table = std::move(table);
	}

	void removeFromExtList(Node* node)
	{
		if (node == m_begin)
		{
			m_begin = m_begin->m_ext.m_next;
			m_begin->m_ext.m_prev = nullptr;
		}
		else
		{
			node->m_ext.m_prev->m_ext.m_next = node->m_ext.m_next;
			node->m_ext.m_next->m_ext.m_prev = node->m_ext.m_prev;
		}
	}

private:
	size_t m_tableSize = 16;
	size_t m_rehashSize = 8;
	kvector<Node*> m_table;
	ContainerAllocator<value_type, ListData> m_allocator;
	Node* m_begin = nullptr;
	const khash<Key> m_hash{};
};
