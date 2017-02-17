/*
 *  signal_tests.cpp
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

#include "simple_tester.h"
#include "su_signal.h"

struct signal_tests
{
	void test_case_1()
	{
		su::signal<int> s;
		int i = 0;
		auto c = s.connect( [&i](int v){ i += v; } );
		TEST_ASSERT_EQUAL( i, 0 );
		s( 5 );
		TEST_ASSERT_EQUAL( i, 5 );
		s( 5 );
		TEST_ASSERT_EQUAL( i, 10 );
		s.disconnect( c );
		s( 5 );
		TEST_ASSERT_EQUAL( i, 10 );
	}

};

REGISTER_TESTS( signal_tests, TEST_CASE(signal_tests,test_case_1) );
