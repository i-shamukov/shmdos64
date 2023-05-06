/*
   kcondition_variable.h
   condition_variable implementation header for SHM DOS64
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
#include <limits>
#include <common_types.h>
#include <kmutex.h>
#include <kevent.h>

class ConditionVariablePrivate;
class KERNEL_SHARED kcondition_variable
{
public:
    static const TimePoint WaitInfinite = kevent::WaitInfinite;
    
public:
    kcondition_variable();
    ~kcondition_variable();
    bool wait(kunique_lock<kmutex>& lock, TimePoint timeout);
    void wait(kunique_lock<kmutex>& lock)
    {
        wait(lock, WaitInfinite);
    }
    void notify_one();
    void notify_all();
    
private:
    kcondition_variable(const kcondition_variable&) = delete;
    kcondition_variable(kcondition_variable&&) = delete;
    kcondition_variable& operator=(const kcondition_variable&) = delete;
    
private:
    ConditionVariablePrivate* m_private;
};
