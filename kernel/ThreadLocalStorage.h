/*
   ThreadLocalStorage.h
   Kernel TLS system data offsets
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
#include <cpu.h>

enum
{
	LOCAL_THREAD_STORAGE_CURRENT_TASK	= 0x000,
	LOCAL_THREAD_STORAGE_SEM_VALUE		= 0x008,
	LOCAL_THREAD_STORAGE_CV_MUTEX		= 0x010,
	LOCAL_THREAD_STORAGE_CV_COUNT		= 0x018,
	LOCAL_THREAD_STORAGE_CV_MUTEX_LOCK	= 0x020,
	LOCAL_THREAD_STORAGE_MUTEX_WAIT		= 0x028,
	LOCAL_THREAD_STORAGE_DATA_SIZE		= PAGE_SIZE
};
