/*
   AbstractTimer.cpp
   Abstract timer implementation
   SHM DOS64
   Copyright (c) 2023, Ilya Shamukov, ilya.shamukov@gmail.com
   
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
#include "TaskManager.h"
#include <conout.h>
static AbstractTimer* g_systemTimer = nullptr;

static bool isSupportRdtscp()
{
   uint32_t eax = CPUID_PROCESSOR_INFOEX_EAX;
   uint32_t ebx;
   uint32_t edx;
   uint32_t ecx;
   cpuCpuid(eax, ebx, edx, ecx);
   return ((edx & CPUID_PROCESSOR_INFOEX_EDX_RDTSCP) != 0);      
}

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
   timer->onInitCpu(BOOT_CPU_ID);
}

void AbstractTimer::updateTimePoint()
{
   if (m_useCpuTsc)
   {
      const unsigned int cpuCount = cpuLogicalCount();
      for (unsigned int cpu = 0; cpu < cpuCount; ++cpu)
         m_cpuTscState[cpu].m_lastCpuTimestamp.store(0, std::memory_order_relaxed);
         
   }
   else
   {
	   m_lastTimepoint.store(timepoint(), std::memory_order_relaxed);
   } 
}

void AbstractTimer::onInterrupt()
{
	AbstractTimer::system()->updateTimePoint();
	TaskManager::onSystemTimerInterrupt();
}

TimePoint AbstractTimer::fastTimepoint()
{
   if (m_useCpuTsc)
   {
      unsigned int cpuId;
      TimePoint tsc = cpuReadTSCP(cpuId);
      for ( ; ; )
      {
         CpuTscState& tscState = m_cpuTscState[cpuId];
         const TimePoint oldTsc = tscState.m_lastCpuTimestamp.load(std::memory_order_acquire);
         if (oldTsc == 0)
         {
            const TimePoint tp = timepoint();
            unsigned int testCpuId;
            tsc = cpuReadTSCP(testCpuId);
            if (testCpuId != cpuId)
            {
               cpuId = testCpuId;
               continue;
            }

            tscState.m_lastCpuTimestamp.store(tsc, std::memory_order_relaxed);
            tscState.m_lastTimerTimestamp.store(tp, std::memory_order_relaxed);
            return tp;
         }

         const TimePoint rest = (tsc - oldTsc) / tscState.m_tscDivider;
         return tscState.m_lastTimerTimestamp.load(std::memory_order_acquire) + rest;
      }
   }

   return m_lastTimepoint.load(std::memory_order_acquire);
}

void AbstractTimer::onInitCpu(unsigned int cpuId)
{
   m_useCpuTsc = m_useCpuTsc && isSupportRdtscp();
   if (!m_useCpuTsc)
      return;
   
   cpuWriteMSR(CPU_MSR_IA32_IA32_TSC_AUX, cpuId);

   const unsigned int measurementsNumber = 100;
   uint32_t tmpCpuId;
   TimePoint startTsc;
   TimePoint endTsc;
   TimePoint startTp;
   TimePoint endTp;
   kvector<TimePoint> measurementsTsc(measurementsNumber);
   kvector<TimePoint> measurementsTp(measurementsNumber);

   for (size_t i = 0; i < measurementsNumber; ++i)
   {
      startTsc = cpuReadTSCP(tmpCpuId);
      startTp = timepoint();
      endTp = timepoint();
      endTsc = cpuReadTSCP(tmpCpuId);
      measurementsTsc[i] = endTsc - startTsc;
      measurementsTp[i] = endTp - startTp;
   }
   std::sort(measurementsTsc.begin(), measurementsTsc.end());
   std::sort(measurementsTp.begin(), measurementsTp.end());

   const TimePoint testLoopDelayTp = measurementsTp[measurementsNumber / 2];
   const TimePoint testLoopDelayTsc = measurementsTsc[measurementsNumber / 2];
   
   uint64_t testWorkLoops = 1024;
   for (size_t i = 0; i < measurementsNumber; ++i)
   {
      startTsc = cpuReadTSCP(tmpCpuId);
      startTp = timepoint();
      for (volatile uint64_t j = 0; j < testWorkLoops; ++j);
      endTp = timepoint();
      endTsc = cpuReadTSCP(tmpCpuId);
      measurementsTsc[i] = endTsc - startTsc - testLoopDelayTsc;
      const TimePoint durTp = endTp - startTp - testLoopDelayTp;
      if (durTp < 100)
      {
         i = std::numeric_limits<size_t>::max();
         testWorkLoops *= 2;
         continue;
      }

      measurementsTsc[i] /= durTp;
   }
   const TimePoint tscDivider = *std::max_element(measurementsTsc.begin(), measurementsTsc.end());
   if (tscDivider < 10)
   {
      m_useCpuTsc = false;
      return;
   }

   CpuTscState& tscState = m_cpuTscState[cpuId];
   tscState.m_tscDivider = tscDivider;
}
