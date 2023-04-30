/*
   klock_guard.h
   Shared header for SHM DOS64
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

template<typename lock_type>
class klock_guard
{
public:
    klock_guard(lock_type& lock)
        : m_lock(lock)
    {
        m_lock.lock();
    }
        
    ~klock_guard()
    {
        m_lock.unlock();
    }

private:
    klock_guard() = delete;
    klock_guard(const klock_guard&) = delete;
    klock_guard(klock_guard&&) = delete;

private:
    lock_type& m_lock;
};


template<typename lock_type>
class kunique_lock
{
public:
    kunique_lock(lock_type& lock)
        : m_lock(lock)
    {
        m_lock.lock();
    }
        
    ~kunique_lock()
    {
        if (m_locked)
            m_lock.unlock();
    }
    
    void lock()
    {
        if (!m_locked)
        {
            m_lock.lock();
            m_locked = true;
        }
    }
    
    void unlock()
    {
        if (m_locked)
        {
            m_locked = false;
            m_lock.unlock();
        }
    }
    
    lock_type* mutex() const
    {
        return &m_lock;
    }

private:
    kunique_lock() = delete;
    kunique_lock(const kunique_lock&) = delete;
    kunique_lock(kunique_lock&&) = delete;

private:
    lock_type& m_lock;
    bool m_locked = true;
};

