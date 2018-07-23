/*
 *  flat_map_tests.cpp
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

#include "su_tests/simple_tests.h"
#include "su_flat_map.h"
#include <ciso646>

struct flat_map_tests
{
	void test_case_1();
};

// register all test cases here
REGISTER_TEST_SUITE( flat_map_tests,
		&flat_map_tests::test_case_1 );

// MARK: -
// MARK:  === test cases ===

struct my_less
{
	int dummy = 0;
	bool operator()( const int &lhs, const int &rhs ) const
		{ return lhs < rhs; }
};

void flat_map_tests::test_case_1()
{
	su::flat_map<int,int> m1;
	su::flat_map<int,int,my_less> m2;
	std::vector<std::pair<int,int>> v;
	m1.reserve( 2 );
	TEST_ASSERT_EQUAL( sizeof(m1), sizeof(v) );
	TEST_ASSERT( sizeof(m2) > sizeof(v) );
}
