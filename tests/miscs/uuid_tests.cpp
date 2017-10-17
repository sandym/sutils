/*
 *  uuid_tests.cpp
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
#include "su/miscs/uuid.h"
#include <ciso646>

struct uuid_tests
{
	void test_case_1()
	{
		auto u1 = su::uuid::create();
		auto u2 = su::uuid::create();
		TEST_ASSERT( not u1.string().empty() );
		TEST_ASSERT( not u2.string().empty() );
		TEST_ASSERT_NOT_EQUAL( u1, u2 );
		TEST_ASSERT_NOT_EQUAL( u1.string(), u2.string() );
		
		su::uuid other( u1.string() );
		TEST_ASSERT_EQUAL( u1, other );
		
		su::uuid a1( "2A3311DD-D62D-415B-966F-99EF085A0000" );
		su::uuid a2( "2A3311DDD62D-415B-966F-99EF085A000" );
		su::uuid a3( "2A3311DD-D62D415B-966F-99EF085A00" );
		su::uuid a4( "2A3311DD-D62D-415B966F-99EF085A0" );
		su::uuid a5( "2A3311DD-D62D-415B-966F-99EF085A" );
		TEST_ASSERT_EQUAL( a1, a2 );
		TEST_ASSERT_EQUAL( a1, a3 );
		TEST_ASSERT_EQUAL( a1, a4 );
		TEST_ASSERT_EQUAL( a1, a5 );
	}
};

REGISTER_TEST_SUITE( uuid_tests, su::timed_test(), &uuid_tests::test_case_1 );
