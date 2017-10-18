/*
 *  mempool_tests.cpp
 *  sutils_tests
 *
 *  Created by Sandy Martel on 2017/03/16.
 *  Copyright 2017 Sandy Martel. All rights reserved.
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
#include "su/miscs/mempool.h"

struct Trivial
{
	void *a, *b, *c;
};

struct mempool_tests
{
	void test_case_1()
	{
		su::mempool<32> mp;
		
		auto p1 = mp.alloc<char>();
		TEST_ASSERT_NOT_EQUAL( p1, nullptr );
		auto p2 = mp.alloc<Trivial>();
		TEST_ASSERT_NOT_EQUAL( p2, nullptr );
		auto p3 = mp.alloc<short>();
		TEST_ASSERT_NOT_EQUAL( p3, nullptr );
		auto p4 = mp.alloc<Trivial>();
		TEST_ASSERT_NOT_EQUAL( p4, nullptr );
		auto p5 = mp.alloc<long>();
		TEST_ASSERT_NOT_EQUAL( p5, nullptr );
		
		//auto p6 = mp.alloc<std::string>(); // this should not compile
	}

};

REGISTER_TEST_SUITE( mempool_tests, &mempool_tests::test_case_1 );
