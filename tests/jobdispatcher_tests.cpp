/*
 *  jobdispatcher_tests.cpp
 *  sutils_tests
 *
 *  Created by Sandy Martel on 2008/05/30.
 *  Copyright 2015 Sandy Martel. All rights reserved.
 *
 *  quick reference:
 *
 *      TEST_ASSERT( condition [, message] )
 *          Assertions that a condition is true.
 *
 *      TEST_FAIL( message )
 *          Fails with the specified message.
 *
 *      TEST_ASSERT_EQUAL( expected, actual [, message] )
 *          Asserts that two values are equals.
 *
 *      TEST_ASSERT_NOT_EQUAL( not expected, actual [, message] )
 *          Asserts that two values are NOT equals.
 *
 *      TEST_ASSERT_EQUAL( expected, actual [, delta, message] )
 *          Asserts that two values are equals.
 *
 *      TEST_ASSERT_NOT_EQUAL( not expected, actual [, delta, message] )
 *          Asserts that two values are NOT equals.
 *
 *      TEST_ASSERT_THROW( expression, ExceptionType [, message] )
 *          Asserts that the given expression throws an exception of the specified type.
 *
 *      TEST_ASSERT_NO_THROW( expression [, message] )
 *          Asserts that the given expression does not throw any exceptions.
 */

#include "simple_tester.h"
#include "su_jobdispatcher.h"
#include "su_job.h"
#include "su_thread.h"
#include <cassert>
#include <ciso646>

class MyDispatcher;

struct jobdispatcher_tests
{
	jobdispatcher_tests();
	~jobdispatcher_tests();

	// declare all test cases here...
	void test_case_async_completion();
	void test_case_no_async_completion();
	void test_case_cancel_1();
	void test_case_cancel_2();
	void test_case_cancel_3();
	void test_case_sprint_1();
	void test_case_sprint_2();
	void test_case_prioritise_1();

	private:
		// declare local members need for the test here
		MyDispatcher *_dispatcher;
		bool _needIdleTime;
};

REGISTER_TESTS( jobdispatcher_tests,
			   TEST_CASE(jobdispatcher_tests,test_case_async_completion),
			   TEST_CASE(jobdispatcher_tests,test_case_no_async_completion),
			   TEST_CASE(jobdispatcher_tests,test_case_cancel_1),
			   TEST_CASE(jobdispatcher_tests,test_case_cancel_2),
			   TEST_CASE(jobdispatcher_tests,test_case_cancel_3),
			   TEST_CASE(jobdispatcher_tests,test_case_sprint_1),
			   TEST_CASE(jobdispatcher_tests,test_case_sprint_2),
			   TEST_CASE(jobdispatcher_tests,test_case_prioritise_1) );

class MyDispatcher : public su::jobdispatcher
{
	public:
		MyDispatcher( bool &io_needIdleTime );
		virtual ~MyDispatcher();
		
		virtual void needIdleTime();
	
		static su::jobdispatcher *g_globalJobDispatcher;

	private:
		bool &_needIdleTime;
};

su::jobdispatcher *MyDispatcher::g_globalJobDispatcher = nullptr;

MyDispatcher::MyDispatcher( bool &io_needIdleTime ) : _needIdleTime( io_needIdleTime )
{
	g_globalJobDispatcher = this;
}

MyDispatcher::~MyDispatcher()
{
	g_globalJobDispatcher = nullptr;
}

void MyDispatcher::needIdleTime()
{
	_needIdleTime = true;
}

su::jobdispatcher *su::jobdispatcher::instance()
{
	assert( MyDispatcher::g_globalJobDispatcher != 0 );
	return MyDispatcher::g_globalJobDispatcher;
}

jobdispatcher_tests::jobdispatcher_tests()
{
	su::this_thread::set_as_main();
	
	_needIdleTime = false;
	_dispatcher = new MyDispatcher( _needIdleTime );
}

jobdispatcher_tests::~jobdispatcher_tests()
{
	// clean up local members
	delete _dispatcher;
	_dispatcher = nullptr;
}

// MARK: -
// MARK:  === test cases ===

class MyAsyncAndCompletionJob : public su::job
{
	public:
		MyAsyncAndCompletionJob( bool &io_completedAsync, bool &io_completed );
		virtual ~MyAsyncAndCompletionJob();
		virtual void runAsync();
		virtual void runIdle();
		
		bool &_completedAsync;
		bool &_completed;
};

MyAsyncAndCompletionJob::MyAsyncAndCompletionJob( bool &io_completedAsync, bool &io_completed )
	: _completedAsync( io_completedAsync ),
		_completed( io_completed )
{
}

MyAsyncAndCompletionJob::~MyAsyncAndCompletionJob()
{
}

void MyAsyncAndCompletionJob::runAsync()
{
	_completedAsync = true;
}
void MyAsyncAndCompletionJob::runIdle()
{
	_completed = true;
}

void jobdispatcher_tests::test_case_async_completion()
{
	bool completedAsync = false, completed = false;
	auto job = std::make_shared<MyAsyncAndCompletionJob>( completedAsync, completed );
	
	TEST_ASSERT( not completedAsync );
	TEST_ASSERT( not completed );
	
	su::jobdispatcher::instance()->postAsync( job );
	su::job_weak_ptr wjob( job );
	job.reset();
	std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	TEST_ASSERT( completedAsync );
	TEST_ASSERT( not completed );
	TEST_ASSERT( not wjob.expired() );
	TEST_ASSERT( _needIdleTime );
	_dispatcher->idle();
	_needIdleTime = false;
	TEST_ASSERT( completed );
	TEST_ASSERT( wjob.expired() );
}

void jobdispatcher_tests::test_case_no_async_completion()
{
	bool completedAsync = false, completed = false;
	auto job = std::make_shared<MyAsyncAndCompletionJob>( completedAsync, completed );
	
	TEST_ASSERT( not completedAsync );
	TEST_ASSERT( not completed );

	su::jobdispatcher::instance()->postIdle( job );
	su::job_weak_ptr wjob( job );
	job.reset();
	std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	TEST_ASSERT( not completedAsync );
	TEST_ASSERT( not completed );
	TEST_ASSERT( not wjob.expired() );
	TEST_ASSERT( _needIdleTime );
	_dispatcher->idle();
	_needIdleTime = false;
	TEST_ASSERT( not completedAsync );
	TEST_ASSERT( completed );
	TEST_ASSERT( wjob.expired() );
}

class LongAsyncJob : public su::job
{
  public:
	LongAsyncJob( int &i_nbFinished, int i_msec );
	virtual ~LongAsyncJob();
	virtual void runAsync();

	int &_nbFinished;
	int _msec;
};

LongAsyncJob::LongAsyncJob( int &i_nbFinished, int i_msec )
	: _nbFinished( i_nbFinished ), 
		_msec( i_msec )
{
}

LongAsyncJob::~LongAsyncJob()
{
	++_nbFinished;
}

void LongAsyncJob::runAsync()
{
	std::this_thread::sleep_for(std::chrono::milliseconds(_msec));
}

void jobdispatcher_tests::test_case_cancel_1()
{
	// cancel while in async queue
	
	// flood the dispatcher with long jobs
	const int nbStarted = 20;
	int	nbFinished = 0;
	for ( int i = 0 ; i < nbStarted; ++i )
		su::jobdispatcher::instance()->postAsync( std::make_shared<LongAsyncJob>( nbFinished, 500 ) );
	
	bool completedAsync = false, completed = false;
	auto job = std::make_shared<MyAsyncAndCompletionJob>( completedAsync, completed );
	
	TEST_ASSERT( not completedAsync );
	TEST_ASSERT( not completed );
	
	su::jobdispatcher::instance()->postAsync( job );
	std::this_thread::sleep_for(std::chrono::milliseconds(250));
	job->cancel();
	su::job_weak_ptr wjob( job );
	job.reset();
	
	while ( nbFinished < nbStarted or _needIdleTime )
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(250));
		if ( _needIdleTime )
		{
			_dispatcher->idle();
			_needIdleTime = false;
		}
	}
	TEST_ASSERT( not completedAsync );
	TEST_ASSERT( not completed );
	TEST_ASSERT( wjob.expired() );
	
}

class TestAsyncCancelJob : public su::job
{
	public:
		TestAsyncCancelJob( bool &i_isRunningAsync, bool &i_isRunningIdle, bool &i_finished );
		virtual ~TestAsyncCancelJob();
		virtual void runAsync();
		virtual void runIdle();
	
		bool &_isRunningAsync;
		bool &_isRunningIdle;
		bool &_finished;
};

TestAsyncCancelJob::TestAsyncCancelJob( bool &i_isRunningAsync, bool &i_isRunningIdle, bool &i_finished )
	: _isRunningAsync( i_isRunningAsync ),
		_isRunningIdle( i_isRunningIdle ),
		_finished( i_finished )
{
}

TestAsyncCancelJob::~TestAsyncCancelJob()
{
}

void TestAsyncCancelJob::runAsync()
{
	_isRunningAsync = true;
	for ( int i = 0; i < 2000; ++i )
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
		cancellationPoint();
	}
	_isRunningAsync = false;
	_finished = true;
}

void TestAsyncCancelJob::runIdle()
{
	_isRunningIdle = true;
}

void jobdispatcher_tests::test_case_cancel_2()
{
	// cancel while running async
	
	// add one very long job
	bool isRunningAsync = false, isRunningIdle = false, finished = false;
	auto longJob = std::make_shared<TestAsyncCancelJob>( isRunningAsync, isRunningIdle, finished );
	su::jobdispatcher::instance()->postAsync( longJob );
	su::job_weak_ptr wjob( longJob );
	
	// flood the dispatcher with long jobs
	const int nbStarted = 20;
	int nbFinished = 0;
	for ( int i = 0 ; i < nbStarted; ++i )
		su::jobdispatcher::instance()->postAsync( std::make_shared<LongAsyncJob>( nbFinished, 500 ) );
	
	std::this_thread::sleep_for(std::chrono::milliseconds(250));
	TEST_ASSERT( isRunningAsync );
	longJob->cancel();
	longJob.reset();
	TEST_ASSERT( not finished );
	TEST_ASSERT( not isRunningIdle );
	
	while ( nbFinished < nbStarted or _needIdleTime )
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(250));
		if ( _needIdleTime )
		{
			_dispatcher->idle();
			_needIdleTime = false;
		}
	}
	TEST_ASSERT( wjob.expired() );
}

void jobdispatcher_tests::test_case_cancel_3()
{
	// cancel while running async with c++ blocks
	
	// add one very long job
	bool isRunningAsync = false, isRunningIdle = false, finished = false;
	auto longJob = std::make_shared<su::asyncJob>(
			[&isRunningAsync, &finished]( su::job *i_job )
			{
				isRunningAsync = true;
				for ( int i = 0; i < 100; ++i )
				{
					std::this_thread::sleep_for(std::chrono::milliseconds(10));
					i_job->cancellationPoint();
				}
				isRunningAsync = false;
				finished = true;
			},
			[&isRunningIdle]( su::job * /*i_job*/ )
			{
				isRunningIdle = true;
			}
		);
	
	su::jobdispatcher::instance()->postAsync( longJob );
	
	// flood the dispatcher with long jobs
	const int nbStarted = 20;
	int	nbFinished = 0;
	for ( int i = 0 ; i < nbStarted; ++i )
	{
		su::jobdispatcher::instance()->postAsync( std::make_shared<su::asyncJob>(
				[]( su::job *i_job )
				{
					for ( int i = 0; i < 100; ++i )
					{
						std::this_thread::sleep_for(std::chrono::milliseconds(10));
						i_job->cancellationPoint();
					}
				},
				[&nbFinished]( su::job * /*i_job*/ )
				{
					++nbFinished;
				}
			) );
	}
	
	std::this_thread::sleep_for(std::chrono::milliseconds(250));
	TEST_ASSERT( isRunningAsync );
	longJob->cancel();
	longJob.reset();
	TEST_ASSERT( not finished );
	TEST_ASSERT( not isRunningIdle );
	
	while ( nbFinished < nbStarted or _needIdleTime )
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(250));
		if ( _needIdleTime )
		{
			_dispatcher->idle();
			_needIdleTime = false;
		}
	}
}

class FirstToFinishJob : public su::job
{
  public:
	FirstToFinishJob( int &i_nbFinished, int &o_result, int i_id );
	virtual ~FirstToFinishJob();
	virtual void runAsync();
	virtual void runIdle();

	int &_nbFinished;
	int &_result;
	int _id;
};

FirstToFinishJob::FirstToFinishJob( int &i_nbFinished, int &o_result, int i_id )
	: _nbFinished( i_nbFinished ),
		_result( o_result ),
		_id( i_id )
{
}

FirstToFinishJob::~FirstToFinishJob()
{
	if ( _result == -1 )
		_result = _id;
	++_nbFinished;
}

void FirstToFinishJob::runAsync()
{
	for ( int i = 0; i < 10; ++i )
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
		cancellationPoint();
	}
}

void FirstToFinishJob::runIdle()
{
	std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

class FirstToFinishJobLog : public FirstToFinishJob
{
	public:
		FirstToFinishJobLog( int &i_nbFinished, int &o_result, bool &i_didAsync, bool &i_didIdle, int i_id );
		virtual ~FirstToFinishJobLog();
		virtual void runAsync();
		virtual void runIdle();
	
		bool &didAsync, &didIdle;
};

FirstToFinishJobLog::FirstToFinishJobLog( int &i_nbFinished, int &o_result, bool &i_didAsync, bool &i_didIdle, int i_id )
	:FirstToFinishJob( i_nbFinished, o_result, i_id ),
		didAsync( i_didAsync ),
		didIdle( i_didIdle )
{
}

FirstToFinishJobLog::~FirstToFinishJobLog()
{
}

void FirstToFinishJobLog::runAsync()
{
	didAsync = true;
	FirstToFinishJob::runAsync();
}

void FirstToFinishJobLog::runIdle()
{
	didIdle = true;
	FirstToFinishJob::runIdle();
}

void jobdispatcher_tests::test_case_sprint_1()
{
	// sprint while in async queue
	
	int winner = -1;
	int nbFinished = 0;
	
	// flood the dispatcher with jobs
	int nbStarted = 20;
	for ( int i = 0 ; i < nbStarted; ++i )
		su::jobdispatcher::instance()->postAsync( std::make_shared<FirstToFinishJob>( nbFinished, winner, 1 ) );
	
	// add one more
	++nbStarted;
	bool didAsync = false, didIdle = false;
	auto job = std::make_shared<FirstToFinishJobLog>( nbFinished, winner, didAsync, didIdle, 2 );
	su::jobdispatcher::instance()->postAsync( job );
	job->sprint();
	job.reset();
	TEST_ASSERT( didAsync );
	TEST_ASSERT( didIdle );
	
	// wait
	while ( nbFinished < nbStarted or _needIdleTime )
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		if ( _needIdleTime )
		{
			_dispatcher->idle();
			_needIdleTime = false;
		}
	}
	TEST_ASSERT_EQUAL( winner, 2 );
}

void jobdispatcher_tests::test_case_sprint_2()
{
	int nbOfIdle = 0;

	auto job = std::make_shared<su::asyncJob>(
			[]( su::job *i_job )
			{
				for ( int i = 0; i < 100; ++i )
				{
					std::this_thread::sleep_for(std::chrono::milliseconds(10));
					i_job->cancellationPoint();
				}
			},
			[&nbOfIdle]( su::job * /*i_job*/ )
			{
				++nbOfIdle;
			}
		);
	su::jobdispatcher::instance()->postAsync( job );
	std::this_thread::sleep_for(std::chrono::milliseconds(200));
	job->sprint();
	TEST_ASSERT_EQUAL( nbOfIdle, 1 );
//	TEST_ASSERT( not _needIdleTime );
	su::jobdispatcher::instance()->idle();
	TEST_ASSERT_EQUAL( nbOfIdle, 1 );
}

void jobdispatcher_tests::test_case_prioritise_1()
{
	// prioritise while in async queue
	
	int winner = -1;
	int nbFinished = 0;
	
	// flood the dispatcher with jobs
	int nbStarted = 20;
	for ( int i = 0 ; i < nbStarted; ++i )
		su::jobdispatcher::instance()->postAsync( std::make_shared<FirstToFinishJob>( nbFinished, winner, 1 ) );
	
	// add one more
	++nbStarted;
	bool didAsync = false, didIdle = false;
	auto job = std::make_shared<FirstToFinishJobLog>( nbFinished, winner, didAsync, didIdle, 2 );
	su::jobdispatcher::instance()->postAsync( job );
	job->prioritise();
	job.reset();
	TEST_ASSERT( not didIdle );
	
	// wait
	int counter = 0, rank = 0;
	while ( nbFinished < nbStarted or _needIdleTime )
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		if ( _needIdleTime )
		{
			_dispatcher->idle();
			++counter;
			_needIdleTime = false;
			if ( didIdle )
				rank = counter;
		}
	}
	TEST_ASSERT( didAsync );
	TEST_ASSERT( rank < 19 ); // rank should be much smaller than 20 since job should have jump ahead of most others.
}
