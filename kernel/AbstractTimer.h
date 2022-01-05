/*
   AbstractTimer.h
   Kernel header
   SHM DOS64
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
#include <atomic>
#include <common_types.h>

class AbstractTimer
{
public:
	AbstractTimer();
	virtual ~AbstractTimer();
	virtual TimeStamp timepoint() const = 0;
	virtual void setEnableInterrupt(bool enable) = 0;
	void updateTimePoint();
	TimeStamp fromSeconds(TimeStamp t) const
	{
		return t * m_frequency;
	}
	TimeStamp fromMilliseconds(TimeStamp t) const
	{
		return t * m_msDivider;
	}
	TimeStamp fromMicroseconds(TimeStamp t) const
	{
		return t * m_usDivider;
	}
	TimeStamp fromNs100(TimeStamp t) const
	{
		return ((m_ns100Divider > 0) ? (t * m_ns100Divider) : ((t * m_usDivider + 9) / 10));
	}
	TimeStamp toSeconds(TimeStamp t) const
	{
		return t / m_frequency;
	}
	TimeStamp toMilliseconds(TimeStamp t) const
	{
		return t / m_msDivider;
	}
	TimeStamp toMicroseconds(TimeStamp t) const
	{
		return t / m_usDivider;
	}
	TimeStamp toNs100(TimeStamp t) const
	{
		return ((m_ns100Divider > 0) ? (t / m_ns100Divider) : (((t + 9) / m_usDivider) * 10));
	}
	TimeStamp fastTimepoint() const
	{
		return m_lastTimepoint.load(std::memory_order_acquire);
	}
	static AbstractTimer* system();
	static void setSystemTimer(AbstractTimer* timer);
	static void onInterrupt();

protected:
	void setFrequency(TimeStamp frequency);

private:
	AbstractTimer(const AbstractTimer&) = delete;
	AbstractTimer(AbstractTimer&&) = delete;

private:
	TimeStamp m_frequency = 0;
	TimeStamp m_msDivider = 0;
	TimeStamp m_usDivider = 0;
	TimeStamp m_ns100Divider = 0;
	std::atomic<TimeStamp> m_lastTimepoint{0};
};
