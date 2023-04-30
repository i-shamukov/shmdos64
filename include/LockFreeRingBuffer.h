/*
   LockFreeRingBuffer.h
   Header-only impementation lock-free RB for SHM DOS64
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
#include <atomic>
#include <memory>

template<typename T>
class LockFreeRingBuffer
{
public:
    LockFreeRingBuffer(size_t size)
        : m_buffer(std::make_unique<Item[]>(size))
        , m_size(size)
    {
		static_assert(std::is_trivial<T>::value);
        for (size_t i = 0; i < size; ++i)
            m_buffer[i].m_counter.store(i, std::memory_order_relaxed);
        m_writeIndex.store(0, std::memory_order_relaxed);
        m_readIndex.store(0, std::memory_order_relaxed);
    }

    bool push(const T & data)
    {
        size_t index = m_writeIndex.load(std::memory_order_relaxed);
		Item* item;
        for (;;)
        {
            item = &m_buffer[index % m_size];
            const size_t cnt = item->m_counter.load(std::memory_order_acquire);
            if (cnt == index)
            {
                if (m_writeIndex.compare_exchange_weak(index, index + 1, std::memory_order_relaxed))
                    break;
            }
            else if (cnt < index)
                return false;
            
			index = m_writeIndex.load(std::memory_order_relaxed);
        }
        item->m_data = data;
        item->m_counter.store(index + 1, std::memory_order_release);
        return true;
    }

    bool pop(T& data)
    {
        Item* item;
        size_t index = m_readIndex.load(std::memory_order_relaxed);
        for (;;)
        {
            item = &m_buffer[index % m_size];
            const size_t cnt = item->m_counter.load(std::memory_order_acquire);
            const size_t nextIndex = index + 1;
            if (cnt == nextIndex)
            {
                if (m_readIndex.compare_exchange_weak(index, nextIndex, std::memory_order_relaxed))
                    break;
            }
            else if (cnt < nextIndex)
                return false;

            index = m_readIndex.load(std::memory_order_relaxed);
        }
        data = item->m_data;
        item->m_counter.store(index + m_size, std::memory_order_release);
        return true;
    }
	
private:
	LockFreeRingBuffer(const LockFreeRingBuffer&) = delete;
	LockFreeRingBuffer(LockFreeRingBuffer&&) = delete;
    void operator=(const LockFreeRingBuffer&);

private:
    struct Item
    {
        std::atomic<size_t> m_counter;
        T m_data;
    };
    std::unique_ptr<Item[]> m_buffer;
    const size_t m_size;
    std::atomic<size_t> m_writeIndex;
    std::atomic<size_t> m_readIndex;
};