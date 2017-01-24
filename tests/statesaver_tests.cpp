/*
 *  statesaver_tests.cpp
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

REGISTER_TESTS( statesaver_tests, 
	TEST_CASE(statesaver_tests,test_case_1),
	TEST_CASE(statesaver_tests,test_case_2) );
