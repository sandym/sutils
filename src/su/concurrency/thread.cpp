//
//  thread.cpp
//  sutils
//
//  Created by Sandy Martel on 12/03/10.
//  Copyright (c) 2015年 Sandy Martel. All rights reserved.
//
// Permission to use, copy, modify, distribute, and sell this software for any
// purpose is hereby granted without fee. The sotware is provided "AS-IS" and
// without warranty of any kind, express, implied or otherwise.
//

#include "su/concurrency/thread.h"
#include "su/base/platform.h"

#if UPLATFORM_WIN
#include <Windows.h>
#endif

namespace {

std::thread::id g_mainThreadID;

}

namespace su {

namespace this_thread {

void set_as_main()
{
	g_mainThreadID = std::this_thread::get_id();
}

bool is_main()
{
	return g_mainThreadID == std::this_thread::get_id();
}

void set_name( const char *n )
{
	if ( n == nullptr or n[0] == 0 )
		return;
	
#if UPLATFORM_WIN
	typedef HRESULT(WINAPI* SetThreadDescriptionPtr)( HANDLE, PCWSTR );

	auto SetThreadDescriptionFunc = reinterpret_cast<SetThreadDescriptionPtr>(
							::GetProcAddress( ::GetModuleHandle(L"Kernel32.dll"), "SetThreadDescription" ) );
	if ( SetThreadDescriptionFunc )
	{
		wchar_t name[64];
		int l = MultiByteToWideChar( CP_UTF8, MB_COMPOSITE, n, strlen(n), name, 64 );
		if ( l > 0 )
		{
			name[l] = 0;
			SetThreadDescriptionFunc( ::GetCurrentThread(), name );
		}
	}
#elif UPLATFORM_MAC
	pthread_setname_np( n );
#else
	pthread_setname_np( pthread_self(), n );
#endif
}

}

void set_low_priority( std::thread &i_thread )
{
#if UPLATFORM_WIN
	SetThreadPriority( i_thread.native_handle(), THREAD_PRIORITY_LOWEST );
#else
	sched_param param;
	param.sched_priority = sched_get_priority_min( SCHED_RR );
	::pthread_setschedparam( i_thread.native_handle(), SCHED_RR, &param );
#endif
}

}
