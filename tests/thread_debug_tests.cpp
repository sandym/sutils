/*
 *  thread_debug_tests.cpp
 *  sutils_tests
 *
 *  Created by Sandy Martel on 2008/05/30.
 *  Copyright 2015 Sandy Martel. All rights reserved.
 *
 *  quick reference:
 *
 *      TEST_ASSERT( condition )
 *          Assertions that a condition is true.
 *
 *      TEST_ASSERT_EQUAL( expected, actual )
 *          Asserts that two values are equals.
 *
 *      TEST_ASSERT_NOT_EQUAL( not expected, actual )
 *          Asserts that two values are NOT equals.
 */

#include "tests/simple_tests.h"
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

REGISTER_TEST_SUITE( thread_debug_tests,
	TEST_CASE(thread_debug_tests,test_case_inMainThread),
	TEST_CASE(thread_debug_tests,test_case_not_inMainThread) );
