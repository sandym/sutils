/*
 *  teebuf_tests.cpp
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
#include "su_teebuf.h"
#include <sstream>

struct teebuf_tests
{
	void test_case_1()
	{
		std::ostringstream os1, os2;
		{
			su::teebuf tb( os1.rdbuf(), os2.rdbuf() );
			std::ostream ostr( &tb );
			
			ostr << 123 << " allo" << std::flush;
		}
		TEST_ASSERT_EQUAL( os1.str(), "123 allo" );
		TEST_ASSERT_EQUAL( os2.str(), "123 allo" );
	}

};

REGISTER_TESTS( teebuf_tests, TEST_CASE(teebuf_tests,test_case_1) );
