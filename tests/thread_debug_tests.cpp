/*
 *  thread_debug_tests.cpp
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
#include <iostream>
#include "su_thread.h"
#include <ciso646>

struct thread_debug_tests
{
	thread_debug_tests()
	{
		su::this_thread::set_as_main();
	}
	
	void test_case_inMainThread()
	{
		TEST_ASSERT( su::this_thread::is_main() );
	}
	void test_case_not_inMainThread()
	{
		bool inMainThread = true;
		std::thread t( [&inMainThread](){ inMainThread = su::this_thread::is_main(); } );
		t.join();
		TEST_ASSERT( not inMainThread );
	}
};

REGISTER_TESTS( thread_debug_tests,
	TEST_CASE(thread_debug_tests,test_case_inMainThread),
	TEST_CASE(thread_debug_tests,test_case_not_inMainThread) );
