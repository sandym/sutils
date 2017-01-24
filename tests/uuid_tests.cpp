/*
 *  uuid_tests.cpp
 *  sutils_tests
 *
 *  Created by Sandy Martel on 2008/05/30.
 *  Copyright 2015 Sandy Martel. All rights reserved.
 *
 *  quick reference:
 *
 *      TEST_ASSERT( condition [, message] )
 *          Assertions that a condition is true.
 *
 *      TEST_FAIL( message )
 *          Fails with the specified message.
 *
 *      TEST_ASSERT_EQUAL( expected, actual [, message] )
 *          Asserts that two values are equals.
 *
 *      TEST_ASSERT_NOT_EQUAL( not expected, actual [, message] )
 *          Asserts that two values are NOT equals.
 *
 *      TEST_ASSERT_EQUAL( expected, actual [, delta, message] )
 *          Asserts that two values are equals.
 *
 *      TEST_ASSERT_NOT_EQUAL( not expected, actual [, delta, message] )
 *          Asserts that two values are NOT equals.
 *
 *      TEST_ASSERT_THROW( expression, ExceptionType [, message] )
 *          Asserts that the given expression throws an exception of the specified type.
 *
 *      TEST_ASSERT_NO_THROW( expression [, message] )
 *          Asserts that the given expression does not throw any exceptions.
 */

#include "simple_tester.h"
#include "su_uuid.h"
#include <ciso646>

struct uuid_tests
{
	void test_case_1()
	{
		auto u1 = su::uuid::create();
		auto u2 = su::uuid::create();
		TEST_ASSERT( not u1.string().empty() );
		TEST_ASSERT( not u2.string().empty() );
		TEST_ASSERT( u1 != u2 );
		TEST_ASSERT( u1.string() != u2.string() );
		
		su::uuid other( u1.string() );
		TEST_ASSERT( u1 == other );
		
		su::uuid a1( "2A3311DD-D62D-415B-966F-99EF085A0000" );
		su::uuid a2( "2A3311DDD62D-415B-966F-99EF085A000" );
		su::uuid a3( "2A3311DD-D62D415B-966F-99EF085A00" );
		su::uuid a4( "2A3311DD-D62D-415B966F-99EF085A0" );
		su::uuid a5( "2A3311DD-D62D-415B-966F-99EF085A" );
		TEST_ASSERT( a1 == a2 );
		TEST_ASSERT( a1 == a3 );
		TEST_ASSERT( a1 == a4 );
		TEST_ASSERT( a1 == a5 );
	}
};

REGISTER_TESTS( uuid_tests, TEST_CASE(uuid_tests,test_case_1) );