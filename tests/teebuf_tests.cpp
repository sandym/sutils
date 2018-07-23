/*
 *  teebuf_tests.cpp
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

#include "su_tests/simple_tests.h"
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

REGISTER_TEST_SUITE( teebuf_tests, &teebuf_tests::test_case_1 );
