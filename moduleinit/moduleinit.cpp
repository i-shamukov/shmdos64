/*
   moduleinit.cpp
   Kernel modules initialization for SHM DOS64
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

extern void onModuleLoad();
extern void onModuleUnload();
extern void onSystemMessage(int msg, int arg, void* ptr);

typedef void (*func_ptr) (void);
extern func_ptr __CTOR_LIST__[];
extern func_ptr __DTOR_LIST__[];

static void __do_global_ctors(void)
{
	__SIZE_TYPE__ nptrs = (__SIZE_TYPE__)__CTOR_LIST__[0];
	if (nptrs == (__SIZE_TYPE__)-1)
	{
		for (nptrs = 0; __CTOR_LIST__[nptrs + 1] != 0; ++nptrs);
	}
	for (unsigned int i = nptrs; i >= 1; --i)
	{
		__CTOR_LIST__[i]();
	}
}

static void __do_global_dtors(void)
{
	for (func_ptr* p = __DTOR_LIST__ + 1; *p != nullptr; ++p)
		(*p)();
}

extern "C" int DllMainCRTStartup(void*, unsigned int reason, void* params)
{
	enum
	{
		moduleUnload = 0,
		moduleLoad = 1,
		systemMsg = 1000
	};
	
	struct Msg
	{
		int m_msg; 
		int m_arg; 
		void* m_ptr;
	};
	
	switch(reason)
	{
	case moduleUnload:
		onModuleUnload();
		__do_global_dtors();
		break;
		
	case moduleLoad:
		__do_global_ctors();
		onModuleLoad();
		break;
		
	case systemMsg:
	{
		Msg* msg = static_cast<Msg*>(params);
		onSystemMessage(msg->m_msg, msg->m_arg, msg->m_ptr);
		break;
	}
	default:
		return 0;
	}
	
	return 1;
}
