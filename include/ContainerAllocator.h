/*
   ContainerAllocator.h
   Container allocation header for SHM DOS64
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
#include <kvector.h>

template<typename T, typename ExtData>
class ContainerAllocator
{
public:
	struct Node
	{
		T m_data;
		ExtData m_ext;
		Node* m_next;
	};

public:
	ContainerAllocator() = default;

	~ContainerAllocator()
	{
		clear();
	}

	template<typename... Args>
	Node* add()
	{
		if (m_freeList == nullptr)
		{
			m_freeList = static_cast<Node*>(operator new[](sizeof(Node) * m_currentChunkSize));
			for (size_t i = 1; i < m_currentChunkSize; ++i)
				m_freeList[i - 1].m_next = &m_freeList[i];
			m_freeList[m_currentChunkSize - 1].m_next = nullptr;
			if (m_currentChunkSize < m_maxChunkSize)
				m_currentChunkSize *= 2;
		}
		Node* ret = m_freeList;
		m_freeList = m_freeList->m_next;
		++m_size;
		return ret;
	}

	void remove(Node* node)
	{
		node->m_next = m_freeList;
		m_freeList = node;
		--m_size;
	}

	void clear()
	{
		m_size = 0;
		m_freeList = nullptr;
		for (Node* node : m_chunk)
			operator delete[](node);
		m_chunk.clear();
	}

	size_t size() const
	{
		return m_size;
	}

private:
	ContainerAllocator(const ContainerAllocator&) = delete;
	ContainerAllocator(ContainerAllocator&&) = delete;
	ContainerAllocator& operator=(const ContainerAllocator&) = delete;

private:
	static const size_t m_maxChunkSize = 0x100000;
	size_t m_currentChunkSize = 0x10;
	Node* m_freeList = nullptr;
	size_t m_size = 0;
	kvector<Node*> m_chunk;
};
