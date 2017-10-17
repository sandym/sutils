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

#include "tests/simple_tests.h"
#include "su/miscs/version.h"
#include <ciso646>

struct version_tests
{
	void test_case_1()
	{
		TEST_ASSERT_EQUAL( su::CURRENT_VERSION().major(), 1 );
		TEST_ASSERT_EQUAL( su::CURRENT_VERSION().minor(), 2 );
		TEST_ASSERT_EQUAL( su::CURRENT_VERSION().patch(), 3 );
		TEST_ASSERT_EQUAL( su::CURRENT_VERSION().build(), 4 );
		TEST_ASSERT( not su::build_revision().empty() );
	}

	void test_case_2()
	{
		// parsing
		auto v = su::version::from_string( "v1-2.3,4xxx" );
		
		TEST_ASSERT_EQUAL( v.major(), 1 );
		TEST_ASSERT_EQUAL( v.minor(), 2 );
		TEST_ASSERT_EQUAL( v.patch(), 3 );
		TEST_ASSERT_EQUAL( v.build(), 4 );
	}

};

REGISTER_TEST_SUITE( version_tests,
	&version_tests::test_case_1,
	&version_tests::test_case_2 );
