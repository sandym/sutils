/*
 *  usersettings_tests.cpp
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
#include "su_usersettings.h"
#include "su_filepath.h"

struct usersettings_tests
{
	usersettings_tests();

	// declare all test cases here...
	void test_case_1();
};

REGISTER_TEST_SUITE( usersettings_tests, TEST_CASE(usersettings_tests,test_case_1) );

usersettings_tests::usersettings_tests()
{
	su::usersettings::init( "sandy", "sutilstests" );
	
	su::usersettings us;
	us.write( "string", "test123" );
	us.write( "bool", true );
	us.write( "int", 123 );
	us.write( "double", 123.5 );
	us.write( "float", 123.5 );
	su::filepath fs( su::filepath::location::kHome );
	us.write( "fs", fs );
	
	std::vector<float> float_vec = { 1, 2, 3, 4 };
	us.write( "float_vec", float_vec );
}

// MARK: -
// MARK:  === test cases ===

void usersettings_tests::test_case_1()
{
	su::usersettings us;
	TEST_ASSERT_EQUAL( us.read<std::string>( "string", "" ), std::string("test123") );
	TEST_ASSERT_EQUAL( us.read<bool>( "bool", false ), true );
	TEST_ASSERT_EQUAL( us.read<int>( "int", 0 ), 123 );
	TEST_ASSERT_EQUAL( us.read<double>( "double", 0 ), double(123.5) );
	TEST_ASSERT_EQUAL( us.read<float>( "float", 0 ), float(123.5) );
	TEST_ASSERT( us.read<su::filepath>( "fs", su::filepath() ).isFolder() );

	std::vector<float> float_vec = { 1, 2, 3, 4 };
	TEST_ASSERT( us.read<std::vector<float>>( "float_vec" ) == float_vec );
}
