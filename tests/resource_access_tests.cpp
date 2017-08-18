/*
 *  resource_access_tests.cpp
 *  sutils
 *
 *  Created by Sandy Martel on 08-05-30.
 *  Copyright (c) 2015年 Sandy Martel. All rights reserved.
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
#include "su_resource_access.h"
#include <ciso646>

struct resource_access_tests
{
	void test_case_getFolder();
};

// register all test cases here
REGISTER_TEST_SUITE( resource_access_tests,
		TEST_CASE(resource_access_tests,test_case_getFolder) );

// MARK: -
// MARK:  === test cases ===

void resource_access_tests::test_case_getFolder()
{
	auto fs = su::resource_access::getFolder();
	TEST_ASSERT( not fs.empty() );
	TEST_ASSERT( fs.isFolder() );
}
