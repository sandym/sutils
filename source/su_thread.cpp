//
//  su_thread.cpp
//  sutils
//
//  Created by Sandy Martel on 12/03/10.
//  Copyright (c) 2015年 Sandy Martel. All rights reserved.
//
// Permission to use, copy, modify, distribute, and sell this software for any purpose is hereby
// granted without fee. The sotware is provided "AS-IS" and without warranty of any kind, express,
// implied or otherwise.
//

#include "su_thread.h"
#include "su_platform.h"

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
#if UPLATFORM_MAC
	pthread_setname_np( n );
#elif !UPLATFORM_WIN
	pthread_setname_np( pthread_self(), n );
#else

#pragma pack(push, 8)
	struct THREADNAME_INFO
	{
		DWORD dwType;
		LPCSTR szName;
		DWORD dwThreadID;
		DWORD dwFlags;
	};
#pragma pack(pop)

	const DWORD kVsThreadNameException = 0x406D1388;

	THREADNAME_INFO info = {0};
	info.dwType = 0x1000;
	info.szName = n;
	info.dwThreadID = GetCurrentThreadId();

	__try
	{
		RaiseException( kVsThreadNameException, 0, sizeof( info ) / sizeof( ULONG_PTR ), (ULONG_PTR *)&info );
	}
	__except ( EXCEPTION_EXECUTE_HANDLER )
	{
	}
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
