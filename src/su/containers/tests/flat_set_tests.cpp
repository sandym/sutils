/*
 *  flat_set_tests.cpp
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

#include "su/tests/simple_tests.h"
#include "su/containers/flat_set.h"
#include <ciso646>

struct flat_set_tests
{
	void test_case_1();
};

// register all test cases here
REGISTER_TEST_SUITE( flat_set_tests,
		&flat_set_tests::test_case_1 );

// MARK: -
// MARK:  === test cases ===

struct my_less : std::binary_function<int, int, bool>
{
	int dummy = 0;
	bool operator()( const int &lhs, const int &rhs ) const
		{ return lhs < rhs; }
};

void flat_set_tests::test_case_1()
{
	su::flat_set<int> s1;
	su::flat_set<int,my_less> s2;
	s1.reserve( 2 );
	std::vector<std::pair<int,int>> v;
	TEST_ASSERT_EQUAL( sizeof(s1), sizeof(v) );
	TEST_ASSERT( sizeof(s2) > sizeof(v) );
}
