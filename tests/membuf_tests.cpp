/*
 *  membuf_tests.cpp
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
#include "su_membuf.h"
#include <sstream>

struct membuf_tests
{
	void test_case_1()
	{
		uint8_t buffer[256];
		for ( int i = 0; i < 256; ++i )
			buffer[i] = (uint8_t)i;
		su::membuf buf( (const char *)buffer, (const char *)buffer + 256 );
		std::istream istr( &buf );
		
		char tmp[4];
		istr.read( tmp, 4 );
		TEST_ASSERT_EQUAL( tmp[0], 0 );
		TEST_ASSERT_EQUAL( tmp[1], 1 );
		TEST_ASSERT_EQUAL( tmp[2], 2 );
		TEST_ASSERT_EQUAL( tmp[3], 3 );
		
		istr.seekg( 25, std::ios::beg );

		istr.read( tmp, 4 );
		TEST_ASSERT_EQUAL( tmp[0], 25 );
		TEST_ASSERT_EQUAL( tmp[1], 26 );
		TEST_ASSERT_EQUAL( tmp[2], 27 );
		TEST_ASSERT_EQUAL( tmp[3], 28 );
	}

};

REGISTER_TEST_SUITE( membuf_tests, &membuf_tests::test_case_1 );
