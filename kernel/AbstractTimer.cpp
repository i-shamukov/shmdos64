/*
   AbstractTimer.cpp
   Abstract timer implementation
   SHM DOS64
   Copyright (c) 2022, Ilya Shamukov, ilya.shamukov@gmail.com
   
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

#include "TaskManager.h"
#include "AbstractTimer.h"

static AbstractTimer* g_systemTimer = nullptr;

AbstractTimer::AbstractTimer()
{

}

AbstractTimer::~AbstractTimer()
{
}

void AbstractTimer::setFrequency(TimePoint frequency)
{
	m_frequency = frequency;
	m_msDivider = frequency / 1000;
	m_usDivider = frequency / 1000'000;
	m_ns100Divider = frequency / 1000'000'0;
}

AbstractTimer* AbstractTimer::system()
{
	return g_systemTimer;
}

void AbstractTimer::setSystemTimer(AbstractTimer* timer)
{
	g_systemTimer = timer;
}

void AbstractTimer::updateTimePoint()
{
	m_lastTimepoint.store(timepoint(), std::memory_order_relaxed);
}

void AbstractTimer::onInterrupt()
{
	AbstractTimer::system()->updateTimePoint();
	TaskManager::onSystemTimerInterrupt();
}
