/*
 *  version_tests.cpp
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
#include "su_version.h"
#include <ciso646>

struct version_tests
{
	void test_case_1()
	{
		TEST_ASSERT_EQUAL( su::CURRENT_VERSION.major(), 1 );
		TEST_ASSERT_EQUAL( su::CURRENT_VERSION.minor(), 2 );
		TEST_ASSERT_EQUAL( su::CURRENT_VERSION.patch(), 3 );
		TEST_ASSERT_EQUAL( su::CURRENT_VERSION.buildNumber(), 4 );
		TEST_ASSERT( not su::CURRENT_VERSION.revision().empty() );
	}

	void test_case_2()
	{
		// parsing
		su::version v( "v1-2.3,4xxx" );
		
		TEST_ASSERT_EQUAL( v.major(), 1 );
		TEST_ASSERT_EQUAL( v.minor(), 2 );
		TEST_ASSERT_EQUAL( v.patch(), 3 );
		TEST_ASSERT_EQUAL( v.buildNumber(), 4 );
		TEST_ASSERT( v.revision().empty() );
	}

};

REGISTER_TESTS( version_tests,
	TEST_CASE(version_tests,test_case_1),
	TEST_CASE(version_tests,test_case_2) );
