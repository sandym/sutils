/*
 *  is_regular_tests.cpp
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
#include "su_is_regular.h"

struct not_default_constructible
{
	not_default_constructible(int){}
};

struct not_copy_constructible
{
	not_copy_constructible(){}
	not_copy_constructible( const not_copy_constructible & ) = delete;
};

struct empty_struct
{
};

struct is_regular_tests
{
	void test_case_1()
	{
		TEST_ASSERT( su::is_regular<int>::value );
		TEST_ASSERT( su::is_regular<empty_struct>::value );
	}

	void test_case_2()
	{
		TEST_ASSERT( not su::is_regular<not_default_constructible>::value );
		TEST_ASSERT( not su::is_regular<not_copy_constructible>::value );
	}
};

REGISTER_TESTS( is_regular_tests,
	TEST_CASE(is_regular_tests,test_case_1),
	TEST_CASE(is_regular_tests,test_case_2) );
