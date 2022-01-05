/*
   InterruptQueue.h
   Shared header for SHM DOS64
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
#include <kevent.h>

class InterruptQueuePrivate;
class InterruptQueue
{
public:
	struct Item
	{
		void* m_obj;
		int m_arg1;
		int m_arg2;
		void* m_data;
	};

public:
	InterruptQueue(size_t size);
	~InterruptQueue();
	bool push(const Item& item);
	bool push(void* obj, int arg1, int arg2, void* data)
	{
		return push(InterruptQueue::Item{obj, arg1, arg2, data});
	}
	bool pop(Item& item, TimeStamp timeout = kevent::WaitInfinite);
	
private:
	InterruptQueue(const InterruptQueue&) = delete;
	InterruptQueue& operator=(const InterruptQueue &) = delete;
	InterruptQueue(InterruptQueue &&) = delete;
	InterruptQueuePrivate* m_private;
};
