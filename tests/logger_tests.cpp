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
#include "su_string.h"
#include "su_thread.h"
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
			su::logger_file lf( su::filepath(tmpLog), su::logger_file::Roll{}, false );

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
			su::logger_file lf( stringLogger, su::filepath(tmpLog), su::logger_file::Roll{}, true );

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

	void test_case_all_type()
	{
		// redirect
		std::ostringstream ss;
		
		{
			su::Logger<> test_logger( ss, "test" );
			
			bool b = true;
			uint8_t u8 = 3;
			char c = 4;
			char buff[] = "1234567890";
			char *ptr = buff;
			
			log_debug(test_logger) << b;
			log_debug(test_logger) << u8;
			log_debug(test_logger) << c;
			log_debug(test_logger) << buff;
			buff[0] = '0';
			log_debug(test_logger) << ptr;
			buff[0] = 'x';
			log_debug(test_logger) << "literal";
		}
		
		auto res = ss.str();
		auto lines = su::split_view( std::string_view{ res }, '\n' );
		TEST_ASSERT_EQUAL( lines.size(), 6 );
		TEST_ASSERT_NOT_EQUAL( lines[0].find( "] true" ), std::string::npos );
		TEST_ASSERT_NOT_EQUAL( lines[1].find( "] 3" ), std::string::npos );
		//TEST_ASSERT_NOT_EQUAL( lines[2].find( "4" ), std::string::npos );
		TEST_ASSERT_NOT_EQUAL( lines[3].find( "] 1234567890" ), std::string::npos );
		TEST_ASSERT_NOT_EQUAL( lines[4].find( "] 0234567890" ), std::string::npos );
		TEST_ASSERT_NOT_EQUAL( lines[5].find( "] literal" ), std::string::npos );
	}

	void test_case_thread_name()
	{
		// redirect
		std::ostringstream ss;
		
		su::this_thread::set_name( "funny_thread_name" );
		{
			su::Logger<> test_logger( ss, "test" );
			
			log_debug(test_logger) << "literal";
		}
		
		auto res = ss.str();
		TEST_ASSERT_NOT_EQUAL( res.find( "[funny_thread_name]" ), std::string::npos );
	}
	
	void test_case_long_message()
	{
		std::ostringstream ss;
		
		{
			su::Logger<> test_logger( ss, "test" );
			
			log_debug(test_logger) << "very long message "
									<< "very long message "
									<< "very long message "
									<< "very long message "
									<< "very long message "
									<< "very long message "
									<< "very long message "
									<< "very long message "
									<< "very long message "
									<< "very long message "
									<< "very long message "
									<< "very long message "
									<< "very long message "
									<< "very long message "
									<< "very long message "
									<< "very long message "
									<< "very long message "
									<< "very long message "
									<< "very long message "
									<< "very long message "
									<< "very long message "
									<< "very long message ";
		}
		
		auto res = ss.str();
		TEST_ASSERT_NOT_EQUAL( res.find( "very long message" ), std::string::npos );
		
		su::log_event ev1(su::kERROR);
		ev1 << "very long message "
									<< "very long message "
									<< "very long message "
									<< "very long message "
									<< "very long message "
									<< "very long message "
									<< "very long message "
									<< "very long message "
									<< "very long message "
									<< "very long message "
									<< "very long message "
									<< "very long message "
									<< "very long message "
									<< "very long message "
									<< "very long message "
									<< "very long message "
									<< "very long message "
									<< "very long message "
									<< "very long message "
									<< "very long message "
									<< "very long message "
									<< "very long message ";
		su::log_event ev2( std::move(ev1) );
	}
};

REGISTER_TEST_SUITE( logger_tests,
	&logger_tests::test_case_1,
	&logger_tests::test_case_2,
	&logger_tests::test_case_3,
	&logger_tests::test_case_4,
	&logger_tests::test_case_5,
	&logger_tests::test_case_all_type,
	&logger_tests::test_case_thread_name,
	&logger_tests::test_case_long_message );
