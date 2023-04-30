/*
   AbstractTimer.h
   Kernel header
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

#pragma once
#include <atomic>
#include <common_types.h>

class AbstractTimer
{
public:
	AbstractTimer();
	virtual ~AbstractTimer();
	virtual TimePoint timepoint() const = 0;
	virtual void setEnableInterrupt(bool enable) = 0;
	void updateTimePoint();
	TimePoint fromSeconds(TimePoint t) const
	{
		return t * m_frequency;
	}
	TimePoint fromMilliseconds(TimePoint t) const
	{
		return t * m_msDivider;
	}
	TimePoint fromMicroseconds(TimePoint t) const
	{
		return t * m_usDivider;
	}
	TimePoint fromNs100(TimePoint t) const
	{
		return ((m_ns100Divider > 0) ? (t * m_ns100Divider) : ((t * m_usDivider + 9) / 10));
	}
	TimePoint toSeconds(TimePoint t) const
	{
		return t / m_frequency;
	}
	TimePoint toMilliseconds(TimePoint t) const
	{
		return t / m_msDivider;
	}
	TimePoint toMicroseconds(TimePoint t) const
	{
		return t / m_usDivider;
	}
	TimePoint toNs100(TimePoint t) const
	{
		return ((m_ns100Divider > 0) ? (t / m_ns100Divider) : (((t + 9) / m_usDivider) * 10));
	}
	TimePoint fastTimepoint() const
	{
		return m_lastTimepoint.load(std::memory_order_acquire);
	}
	static AbstractTimer* system();
	static void setSystemTimer(AbstractTimer* timer);
	static void onInterrupt();

protected:
	void setFrequency(TimePoint frequency);

private:
	AbstractTimer(const AbstractTimer&) = delete;
	AbstractTimer(AbstractTimer&&) = delete;

private:
	TimePoint m_frequency = 0;
	TimePoint m_msDivider = 0;
	TimePoint m_usDivider = 0;
	TimePoint m_ns100Divider = 0;
	std::atomic<TimePoint> m_lastTimepoint{0};
};
