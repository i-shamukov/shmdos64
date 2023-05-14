/*
   kshared_mutex.h
   Kernel shared mutex
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
#include <kcondition_variable.h>
#include <kmutex.h>

class kshared_mutex
{
public:
    kshared_mutex() { }
    
    void shared_lock()
    {
        kunique_lock lock(m_mtx);
        while (m_lock)
        {
            m_wait = true;
            m_cv.wait(lock);
        }
        ++m_sharedCount;
    }
    
    void shared_unlock()
    {
        bool notify = false;
        {
            klock_guard lock(m_mtx);
            --m_sharedCount;
            if (m_wait && (m_sharedCount == 0))
            {
                m_wait = false;
                notify = true;
            }
        }
        if (notify)
            m_cv.notify_one();
    }

    void lock()
    {
        kunique_lock lock(m_mtx);
        while (m_lock || (m_sharedCount != 0))
        {
            m_wait = true;
            m_cv.wait(lock);
        }
        m_lock = true;
    }

    void unlock()
    {
        bool notify = false;
        {
            klock_guard lock(m_mtx);
            m_lock = false; 
            if (m_wait)
            {
                m_wait = false;
                notify = true;
            }
        }
        if (notify)
            m_cv.notify_all();
    }

private:
    kshared_mutex(const kshared_mutex&) = delete;
    kshared_mutex(kshared_mutex&&) = delete;
    kshared_mutex& operator=(const kshared_mutex&) = delete;

private:
    bool m_lock = false;
    bool m_wait = false;
    int m_sharedCount = 0;
    kmutex m_mtx;
    kcondition_variable m_cv;
};

template<typename SharedMutexType>
class klock_shared
{
public:
    klock_shared(SharedMutexType& mtx)
        : m_mtx(mtx)
    {
        m_mtx.shared_lock();
    }

    ~klock_shared()
    {
        m_mtx.shared_unlock();
    }

private:
    klock_shared(const klock_shared&) = delete;
    klock_shared(klock_shared&&) = delete;
    klock_shared& operator=(const klock_shared&) = delete;

private:
    SharedMutexType& m_mtx;
};
