/*
 *  string_load_tests.cpp
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

#include "su/tests/simple_tests.h"
#include "su/strings/str_load.h"

struct string_load_tests
{
	//	declare all test cases here...
	void test_case_1();
};

REGISTER_TEST_SUITE( string_load_tests,
				&string_load_tests::test_case_1 );

// MARK: -
// MARK:  === test cases ===

#include "su/base/cfauto.h"

void string_load_tests::test_case_1()
{
	auto s = su::string_load( "key", "test" );
	TEST_ASSERT_EQUAL( s, "localised key" );
	s = su::string_load( "does not exists", "test" );
	TEST_ASSERT_EQUAL( s, "does not exists" );
}
