/*
 *  logger_tests.cpp
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
#include "su_logger.h"
#include "su_logger_file.h"
#include "su_filepath.h"
#include "su_platform.h"
#include <iostream>
#include <sstream>
#if UPLATFORM_WIN
#include <Windows.h>
#endif

struct logger_tests
{
	void test_case_1()
	{
		// redirect
		std::ostringstream ss;
		
		{
			su::Logger<> test_logger( ss, "test" );
			log_debug(test_logger) << 3;
		}
		
		auto res = ss.str();
		TEST_ASSERT_NOT_EQUAL( res.find( "] 3\n" ), std::string::npos );
		TEST_ASSERT_NOT_EQUAL( res.find( "test_case_1" ), std::string::npos );
		TEST_ASSERT_NOT_EQUAL( res.find( "[test]" ), std::string::npos );
	}

	void test_case_2()
	{
		// redirect
		auto old = std::clog.rdbuf();
		std::ostringstream ss;
		std::clog.rdbuf( ss.rdbuf() );
		
		log_debug() << "test " << 4;
		std::this_thread::sleep_for( std::chrono::milliseconds(50) );

		// restore
		std::clog.rdbuf( old );
		
		auto res = ss.str();
		TEST_ASSERT_NOT_EQUAL( res.find( "test 4\n" ), std::string::npos );
		TEST_ASSERT_NOT_EQUAL( res.find( "DEBUG" ), std::string::npos );
	}

	void test_case_3()
	{
#if UPLATFORM_WIN
		char tmpLog[MAX_PATH + 1];
		TEST_ASSERT( GetTempPath(MAX_PATH, tmpLog) != 0 );
		strcat(tmpLog, "sutils_tests.log");
#else
		const char *tmpLog = "/tmp/sutils_tests.log";
#endif

		{
			su::logger_file lf( tmpLog, false, su::logger_file::action::kPush );

			log_debug() << "test " << 1;
			log_warn() << "test " << 3;
		}
		
		std::ifstream istr( tmpLog );
		
		if ( not istr )
			TEST_ASSERT( !"cannot read /tmp/sutils_tests.log" );
		
		std::string line;

		std::getline( istr, line );
		TEST_ASSERT_NOT_EQUAL( line.find( "test 1" ), std::string::npos );
		std::getline( istr, line );
		TEST_ASSERT_NOT_EQUAL( line.find( "test 3" ), std::string::npos );
	}

	void test_case_4()
	{
		std::ostringstream ss;
		{
			su::Logger<> test_logger( ss, "test" );
			log_info(test_logger) << "123 " << 2;
		}
		
		auto res = ss.str();
		TEST_ASSERT_NOT_EQUAL( res.find( "123 2\n" ), std::string::npos );
		TEST_ASSERT_NOT_EQUAL( res.find( "[test]" ), std::string::npos );
		TEST_ASSERT_NOT_EQUAL( res.find( "[INFO]" ), std::string::npos );
	}
};

REGISTER_TESTS( logger_tests,
	TEST_CASE(logger_tests,test_case_1),
	TEST_CASE(logger_tests,test_case_2),
	TEST_CASE(logger_tests,test_case_3),
	TEST_CASE(logger_tests,test_case_4) );
