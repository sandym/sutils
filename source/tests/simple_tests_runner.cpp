//
//  simple_tests_runner.h
//  sutils
//
//  Created by Sandy Martel on 12/03/10.
//  Copyright (c) 2015年 Sandy Martel. All rights reserved.
//
// Permission to use, copy, modify, distribute, and sell this software for any purpose is hereby
// granted without fee. The sotware is provided "AS-IS" and without warranty of any kind, express,
// implied or otherwise.

#include "tests/simple_tests.h"
#include "su_platform.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <chrono>
#include <map>
#include <string.h>
#include <ciso646>

#if !UPLATFORM_WIN
#include <unistd.h>
#endif

#ifdef HAS_SQLITE3
#include <sqlite3.h>
#endif

namespace
{
//! all the test suites
std::vector<su::TestSuiteAbstract *> *g_testSuites;

bool g_supportColour = false;
void checkTTYColourSupport()
{
#if !UPLATFORM_WIN
	g_supportColour = isatty(1);
	if ( g_supportColour )
	{
		auto n = ttyname(1);
		if ( n and n[strlen(n)-1] != '1' )
			g_supportColour = false;
	}
#endif
}

//! nicer display name, for test suites and test cases
std::string displayName( const std::string &s )
{
	std::string result( s );
	if ( result.compare( 0, 6, "timed_" ) == 0 )
		result.erase( 0, 6 );
	std::replace( result.begin(), result.end(), '_', ' ' );
	return result;
}

const char *reset()
{
	if ( not g_supportColour )
		return "";
	return "\x1b[0m";
}

const char *underline()
{
	if ( not g_supportColour )
		return "";
	return "\x1b[4m";
}
const char *bold()
{
	if ( not g_supportColour )
		return "";
	return "\x1b[1m";
}
const char *green()
{
	if ( not g_supportColour )
		return "";
	return "\x1b[32m";
}
const char *red()
{
	if ( not g_supportColour )
		return "";
	return "\x1b[31m";
}

class SimpleTestDB
{
public:
	SimpleTestDB( const std::string &i_processName );
	~SimpleTestDB();
	
	void addResult( const std::string &i_testSuiteName, const std::string &i_testName, const std::string &i_result, int64_t i_duration );
	int64_t mostRecentDuration( const std::string &i_testSuiteName, const std::string &i_testName );

private:
	std::string _processName;

#ifdef HAS_SQLITE3
	sqlite3 *_db = nullptr;
	std::string _datetime;
	
	static int sqlite3_exec_callback( void* data, int nb, char** values, char** columns )
	{
		auto result = (std::map<std::string,std::string> *)data;
		for ( int i = 0; i < nb; ++i )
		{
			if ( columns[i] and values[i] )
				(*result)[columns[i]] = values[i];
		}
		return 0;
	}
	std::map<std::string,std::string> sqlite3_exec( const std::string &i_cmd )
	{
		std::map<std::string,std::string> result;
		if ( _db )
		{
			char *errmsg = nullptr;
			int err = ::sqlite3_exec( _db, i_cmd.c_str(), &SimpleTestDB::sqlite3_exec_callback, &result, &errmsg );
			if ( err != SQLITE_OK )
			{
				throw std::runtime_error( errmsg );
			}
			if ( errmsg )
				sqlite3_free( errmsg );
		}
		return result;
	}
#endif
};

SimpleTestDB::SimpleTestDB( const std::string &i_processName )
	: _processName( i_processName )
{
#ifdef HAS_SQLITE3
	std::string path;
	auto parent = getenv( "LOCALAPPDATA" ); // for windows
	if ( parent == nullptr or *parent == 0 )
		parent = getenv( "HOME" ); // others
	if ( parent != nullptr )
	{
		path.assign( parent );
		path += "/.simple_tests.db";
	}

	if ( path.empty() )
	{
		std::cerr << "error: cannot find a path for the database" << std::endl;
		return;
	}
	
	try
	{
		int err = sqlite3_open( path.c_str(), &_db );
		if ( err == SQLITE_OK )
		{
			sqlite3_exec( "CREATE TABLE IF NOT EXISTS test_cases ("
								"process_name TEXT,"
								"test_suite TEXT,"
								"test_case TEXT,"
								"result TEXT,"
								"duration INT,"
								"datetime TEXT"
								");" );
			auto result = sqlite3_exec( "SELECT datetime('now','utc');" );
			if ( not result.empty() )
				_datetime = result.begin()->second;
		}
	}
	catch ( std::exception &ex )
	{
		std::cerr << "opening database failed: " << ex.what() << std::endl;
	}
	std::cout << bold() << "Tests database: " << reset() << path << std::endl;
#endif
}

SimpleTestDB::~SimpleTestDB()
{
#ifdef HAS_SQLITE3
	if ( _db )
		sqlite3_close( _db );
#endif
}

void SimpleTestDB::addResult( const std::string &i_testSuiteName, const std::string &i_testName, const std::string &i_result, int64_t i_duration )
{
#ifdef HAS_SQLITE3
	try
	{
		std::string cmd( "INSERT INTO test_cases (process_name,test_suite,test_case,result,duration,datetime) VALUES (" );
		cmd += "'" + _processName + "',";
		cmd += "'" + i_testSuiteName + "',";
		cmd += "'" + i_testName + "',";
		cmd += "'" + i_result + "',";
		cmd += std::to_string(i_duration) + ",";
		cmd += "'" + _datetime;
		cmd += "');";
		sqlite3_exec( cmd );
	}
	catch ( std::exception &ex )
	{
		std::cerr << "inserting in database failed: " << ex.what() << std::endl;
	}
#endif
}

int64_t SimpleTestDB::mostRecentDuration( const std::string &i_testSuiteName, const std::string &i_testName )
{
#ifdef HAS_SQLITE3
	try
	{
		std::string cmd( "SELECT duration FROM test_cases WHERE " );
		cmd += "process_name='" + _processName + "' AND ";
		cmd += "test_suite='" + i_testSuiteName + "' AND ";
		cmd += "test_case='" + i_testName + "' AND ";
		cmd += "result='' ";
		cmd += "ORDER BY datetime DESC LIMIT 1";
		auto result = sqlite3_exec( cmd );
		if ( not result.empty() )
			return std::stoll( result.begin()->second );
	}
	catch ( std::exception &ex )
	{
		std::cerr << "inserting in database failed: " << ex.what() << std::endl;
	}
#endif
	return std::numeric_limits<int64_t>::max();
}

}

namespace su
{

void addTestSuite( TestSuiteAbstract *i_tg )
{
	if ( g_testSuites == nullptr )
		g_testSuites = new std::vector<su::TestSuiteAbstract *>;
	g_testSuites->push_back( i_tg );
}

FailedTest::FailedTest( Type i_type, const char *i_file, int i_line, const std::string &i_text, const std::string &i_msg )
	: _type( i_type ),
		_file( i_file ),
		_line( i_line ),
		_text( i_text ),
		_msg( i_msg )
{
}

const char *FailedTest::what() const noexcept
{
	if ( _storage.empty() )
	{
		std::ostringstream str;
		switch ( _type )
		{
			case Type::kAssert: str << "ASSERTION("; break;
			case Type::kEqual: str << "EQUAL("; break;
			case Type::kNotEqual: str << "NOT EQUAL("; break;
			default: break;
		}
		str << _text << ")\n";
		str << _file << ":" << _line << "\n";
		if ( not _msg.empty() )
			str << _msg << "\n";
		_storage = str.str();
	}
	return _storage.c_str();
}

}

int main( int argc, char **argv )
{
	//	remove the path base, just find the process name
	const char *processName = argv[0];
	std::reverse_iterator<const char *> begin( processName + strlen( processName ) );
	std::reverse_iterator<const char *> end( processName );
	std::reverse_iterator<const char *> pos = std::find_if( begin, end, []( char c ){ return c == '/' or c == '\\'; } );
	if ( pos != end )
		processName = pos.base();
	
	checkTTYColourSupport();
	
	SimpleTestDB db( processName );
	
	int total = 0;
	int failure = 0;
	for ( auto testSuite : *g_testSuites )
	{
		std::cout << underline() << bold() << "Test case:" << reset() << " "  << displayName(testSuite->name()) << std::endl;
		auto tests = testSuite->getTests();
		for ( auto test : tests )
		{
			++total;
			
			std::cout << "  " << displayName(test.name()) << " : ";
			std::cout.flush();
			
			int64_t duration;
			std::string result;
			try
			{
				auto start_time = std::chrono::high_resolution_clock::now();
				test();
				auto end_time = std::chrono::high_resolution_clock::now();
				duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time).count();
				std::cout << green() << bold() << "OK" << reset();
				if ( test.timed() )
				{
					auto prev = db.mostRecentDuration( testSuite->name(), test.name() );
					std::cout << " (" << duration << "ns";
					if ( duration > prev )
						std::cout << red() << " regression: " << duration - prev << "ns slower" << reset();
					std::cout << ")";
				}
			}
			catch ( su::FailedTest &ex )
			{
				result = std::string("TEST FAIL - ") + ex.what();
				++failure;
			}
			catch ( std::exception &ex )
			{
				result = std::string("EXCEPTION CAUGHT - ") + ex.what();
				++failure;
			}
			catch ( ... )
			{
				result = "UNKNOWN EXCEPTION CAUGHT";
				++failure;
			}
			if ( not result.empty() )
				std::cout << red() << result << reset();
			std::cout << std::endl;
			
			db.addResult( testSuite->name(), test.name(), result, duration );
		}
	}
	std::cout << bold() << "Success: " << reset() << (total-failure) << "/" << total << std::endl;
	if ( failure == 0 )
		std::cout << bold() << green() << "All good!" << reset() << std::endl;
	std::cout << std::endl;
}
