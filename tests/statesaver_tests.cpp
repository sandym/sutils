/*
 *  statesaver_tests.cpp
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
#include "su_statesaver.h"

struct statesaver_tests
{
	void test_case_1()
	{
		int v = 3;
		su::statesaver<int> ss( v, 4 );
		TEST_ASSERT_EQUAL( v, 4 );
		ss.restore();
		TEST_ASSERT_EQUAL( v, 3 );
	}

	void test_case_2()
	{
		int v = 3;
		su::statesaver<int> ss( v );
		v = 4;
		TEST_ASSERT_EQUAL( v, 4 );
		ss.restore();
		TEST_ASSERT_EQUAL( v, 3 );
	}

};

REGISTER_TEST_SUITE( statesaver_tests, 
	&statesaver_tests::test_case_1,
	&statesaver_tests::test_case_2 );
