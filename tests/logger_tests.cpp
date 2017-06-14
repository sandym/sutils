/*
 *  logger_tests.cpp
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
	const char *get_tmp_file() const
	{
#if UPLATFORM_WIN
		static char tmpLog[MAX_PATH + 1];
		TEST_ASSERT( GetTempPath(MAX_PATH, tmpLog) != 0 );
		strcat(tmpLog, "sutils_tests.log");
#else
		const char *tmpLog = "/tmp/sutils_tests.log";
#endif
		return tmpLog;
	}
	
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
		auto tmpLog = get_tmp_file();

		{
			su::logger_file lf( tmpLog, su::logger_file::Roll{}, false );

			log_debug() << "test " << 1;
			log_warn() << "test " << 3;
		}
		
		std::ifstream istr( tmpLog );
		
		if ( not istr )
			TEST_FAIL( std::string("cannot read ") + tmpLog );
		
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

	void test_case_5()
	{
		auto tmpLog = get_tmp_file();
		
		std::ostringstream osstr;
		su::Logger<> stringLogger( osstr );
		log_debug(stringLogger) << "first!";
		
		{
			su::logger_file lf( stringLogger, tmpLog, su::logger_file::Roll{}, true );

			log_debug(stringLogger) << "test " << 1;
			log_warn(stringLogger) << "test " << 3;
		}
		
		std::ifstream istr( tmpLog );
		
		if ( not istr )
			TEST_FAIL( std::string("cannot read ") + tmpLog );
		
		std::string line;

		std::getline( istr, line );
		TEST_ASSERT_NOT_EQUAL( line.find( "test 1" ), std::string::npos );
		std::getline( istr, line );
		TEST_ASSERT_NOT_EQUAL( line.find( "test 3" ), std::string::npos );
		
		// osstr should also contains the log
		auto s = osstr.str();
		TEST_ASSERT_NOT_EQUAL( s.find( "test 1" ), std::string::npos );
		TEST_ASSERT_NOT_EQUAL( s.find( "test 3" ), std::string::npos );
	}

};

REGISTER_TEST_SUITE( logger_tests,
	TEST_CASE(logger_tests,test_case_1),
	TEST_CASE(logger_tests,test_case_2),
	TEST_CASE(logger_tests,test_case_3),
	TEST_CASE(logger_tests,test_case_4),
	TEST_CASE(logger_tests,test_case_5) );
