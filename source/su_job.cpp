//
//  su_job.cpp
//  sutils
//
//  Created by Sandy Martel on 12/03/04.
//  Copyright (c) 2015年 Sandy Martel. All rights reserved.
//
//	Permission to use, copy, modify, distribute, and sell this software for any purpose is hereby
//	granted without fee. The sotware is provided "AS-IS" and without warranty of any kind, express,
//	implied or otherwise.
//

#include "su_job.h"
#include "su_jobdispatcher.h"
#include "su_thread.h"

namespace su {

job::~job()
{
	AssertIsMainThread();
}

void job::runAsync()
{
}

void job::runIdle()
{
	AssertIsMainThread();
}

void job::cancel()
{
	AssertIsMainThread();
	assert( _dispatcher != nullptr );
	_dispatcher->cancel_impl( shared_from_this() );
}

void job::sprint()
{
	AssertIsMainThread();
	assert( _dispatcher != nullptr );
	_dispatcher->sprint_impl( shared_from_this() );
}

void job::prioritise()
{
	AssertIsMainThread();
	assert( _dispatcher != nullptr );
	_dispatcher->prioritise_impl( shared_from_this() );
}

#if 0
#pragma mark -
#endif

asyncJob::asyncJob( const std::function< void(job*) > &i_async, const std::function< void(job*) > &i_idle )
	: _async( i_async ), _idle( i_idle )
{
}

asyncJob::~asyncJob()
{
}

void asyncJob::runAsync()
{
	if ( _async )
		_async( this );
}

void asyncJob::runIdle()
{
	if ( _idle )
		_idle( this );
}

}
