//
//  su_jobdispatcher.cpp
//  sutils
//
//  Created by Sandy Martel on 12/03/04.
//  Copyright (c) 2015年 Sandy Martel. All rights reserved.
//
// Permission to use, copy, modify, distribute, and sell this software for any
// purpose is hereby granted without fee. The sotware is provided "AS-IS" and
// without warranty of any kind, express, implied or otherwise.
//

#include "su_jobdispatcher.h"
#include "su_job.h"
#include "su_thread.h"
#include <ciso646>
#include <memory>
#include <algorithm>

namespace su {

jobdispatcher::jobdispatcher( int i_nbOfWorkers )
{
	AssertIsMainThread();
	if ( i_nbOfWorkers < 1 )
	{
		// one less than the number of cpu, no smaller than 1!
		i_nbOfWorkers =
		    std::max<unsigned>( std::thread::hardware_concurrency() - 1, 1 );
	}
	// just reserve the space, we'll lazily start the threads later
	_threadPool.resize( i_nbOfWorkers );
}

jobdispatcher::~jobdispatcher()
{
	AssertIsMainThread();

	std::unique_lock<std::mutex> l( _JDMutex );
	_isRunning = false;

	// remove all non-running jobs
	_asyncQueue.remove_if(
	    []( const job_ptr &j ) { return j->_threadIndex == -1; } );

	// cancel all running jobs
	while ( not _asyncQueue.empty() )
	{
		cancel_with_lock_impl( _asyncQueue.front() );
	}

	l.unlock();
	_JDCond.notify_all();

	// wait for all threads
	for ( auto &it : _threadPool )
	{
		if ( it.get() != nullptr )
			it->join();
	}
}

bool jobdispatcher::removeFromQueue( std::list<job_ptr> &io_queue,
                                     const job_ptr &i_job )
{
	auto it = std::find( io_queue.begin(), io_queue.end(), i_job );
	if ( it != io_queue.end() )
	{
		io_queue.erase( it );
		return true;
	}
	return false;
}

bool jobdispatcher::moveToFrontOfQueue( std::list<job_ptr> &io_queue,
                                        const job_ptr &i_job )
{
	auto it = std::find( io_queue.begin(), io_queue.end(), i_job );
	if ( it != io_queue.end() )
	{
		io_queue.splice( io_queue.begin(), io_queue, it );
		return true;
	}
	return false;
}

void jobdispatcher::postAsync( const job_ptr &i_job )
{
	AssertIsMainThread();
	assert( i_job.get() != nullptr );
	i_job->_cancelled.store( false );

	// allocate list node
	std::list<job_ptr> t;
	t.push_back( i_job );

	std::unique_lock<std::mutex> l( _JDMutex );

	// lazily start workers as needed
	if ( _nbOfWorkersFree == 0 and
	     _nbOfWorkersRunning < (int)_threadPool.size() )
	{
		_threadPool[_nbOfWorkersRunning] = std::make_unique<std::thread>(
		    &jobdispatcher::workerThread, this, _nbOfWorkersRunning );
		++_nbOfWorkersRunning;
	}

	// move list node, no allocations
	_asyncQueue.splice( _asyncQueue.end(), t );
	i_job->_dispatcher = this;

	_JDCond.notify_one();
}

void jobdispatcher::postIdle( const job_ptr &i_job )
{
	AssertIsMainThread();
	assert( i_job.get() != nullptr );
	i_job->_cancelled.store( false );

	std::list<job_ptr> t;
	t.push_back( i_job );

	// just splice, no memory allocation
	std::unique_lock<std::mutex> l( _JDMutex );
	_idleQueue.splice( _idleQueue.end(), t );

	if ( _idleQueue.size() == 1 )
	{
		l.unlock();
		needIdleTime();
	}
}

void jobdispatcher::workerThread( int i_threadIndex )
{
	su::this_thread::set_name( "worker" );

	try
	{
		AssertIsNotMainThread();
		std::unique_lock<std::mutex> l( _JDMutex );
		while ( _isRunning )
		{
			// mutex is locked here
			++_nbOfWorkersFree;
			_JDCond.wait( l, [this]() {
				return not this->_asyncQueue.empty() or not this->_isRunning;
			} );
			--_nbOfWorkersFree;

			if ( _isRunning )
			{
				// get first in queue
				auto job = _asyncQueue.front();

				// remove from queue
				_asyncQueue.pop_front();

				assert( not job->_cancelled.load() );

				// record the thread it will run on
				job->_threadIndex = i_threadIndex;

				l.unlock();

				// run it
				try
				{
					job->runAsync();
				}
				catch ( ... )
				{
				}

				// push it, even if it was cancelled,
				// so that it gets destroyed on the main thread

				// allocate a list node here, while mutex is unlock
				std::list<job_ptr> t;
				t.push_back( job );

				l.lock();

				// remove from thread
				job->_threadIndex = -1;

				// post for completion and be deleted
				_idleQueue.splice( _idleQueue.end(), t );

				// notify in case someone is waiting on this
				job->_cond.notify_all();
				job.reset();

				if ( _idleQueue.size() == 1 )
				{
					l.unlock();
					needIdleTime();
					l.lock();
				}
			}
		}
	}
	catch ( ... )
	{
		assert( false ); // we should get out of here normally... no exceptions
	}
}

void jobdispatcher::cancel_impl( const job_ptr &i_job )
{
	AssertIsMainThread();
	std::unique_lock<std::mutex> l( _JDMutex );
	cancel_with_lock_impl( i_job );
}

void jobdispatcher::cancel_with_lock_impl( const job_ptr &i_job )
{
	assert( not i_job->_cancelled.load() );
	assert( i_job->_dispatcher == this );
	i_job->_cancelled.store( true );
	if ( i_job->_threadIndex == -1 )
	{
		// look for it in the async or idle queue
		bool wasInAsyncQueue = removeFromQueue( _asyncQueue, i_job );
		bool wasInIdleQueue =
		    not wasInAsyncQueue ? removeFromQueue( _idleQueue, i_job ) : false;
		if ( not wasInAsyncQueue and not wasInIdleQueue )
		{
			assert( false ); // could it be in no queue?
		}
	}
	// else it will be collected at idle
}

void jobdispatcher::sprint_impl( const job_ptr &i_job )
{
	AssertIsMainThread();
	std::unique_lock<std::mutex> l( _JDMutex );
	assert( not i_job->_cancelled.load() );
	assert( i_job->_dispatcher == this );
	if ( i_job->_threadIndex != -1 )
	{
		// async part is currently running, wait for it
		i_job->_cond.wait( l, [i_job]() { return i_job->_threadIndex == -1; } );

		// finish the job right away
		bool wasInIdleQueue [[maybe_unused]] =
		    removeFromQueue( _idleQueue, i_job );
		l.unlock();
		assert( wasInIdleQueue );
		i_job->runIdle();
	}
	else
	{
		// look for it in the async or idle queue
		bool wasInAsyncQueue = removeFromQueue( _asyncQueue, i_job );
		bool wasInIdleQueue [[maybe_unused]] =
		    wasInAsyncQueue ? false : removeFromQueue( _idleQueue, i_job );
		l.unlock();
		assert( wasInAsyncQueue or wasInIdleQueue ); // could it be in no queue?

		// do the job the right away
		if ( wasInAsyncQueue )
			i_job->runAsync();

		i_job->runIdle();
	}
}

void jobdispatcher::prioritise_impl( const job_ptr &i_job )
{
	AssertIsMainThread();
	std::unique_lock<std::mutex> l( _JDMutex );
	assert( not i_job->_cancelled.load() );
	assert( i_job->_dispatcher == this );
	if ( i_job->_threadIndex == -1 )
	{
		// look for it in the async queue
		if ( not moveToFrontOfQueue( _asyncQueue, i_job ) )
		{
			// look for it in the idle queue
			moveToFrontOfQueue( _idleQueue, i_job );
		}
	}
}

void jobdispatcher::idle()
{
	AssertIsMainThread();
	std::unique_lock<std::mutex> l( _JDMutex );
	while ( not _idleQueue.empty() )
	{
		// we have jobs, move them all to a new queue for
		// execution and unlock the main queue

		std::list<job_ptr> jobs;
		jobs.swap( _idleQueue );
		l.unlock();

		// complete the jobs
		for ( auto &job : jobs )
		{
			if ( not job->_cancelled.load() )
				job->runIdle();
			job.reset();
		}
		l.lock();
	}
}

void jobdispatcher::needIdleTime() {}

}
