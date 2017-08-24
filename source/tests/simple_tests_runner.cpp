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
#include <ciso646>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <map>
#include <set>
#include <string.h>

#if __has_include(<sqlite3.h>)
#define HAS_SQLITE3
#include <sqlite3.h>
#endif

namespace {

enum
{
	ttyRed = 1,
	ttyGreen,
	ttyBlue,
	ttyCyan,
	ttyMagenta,
	ttyYellow,
	ttyBlack,
	ttyWhite,

	ttyColourMask = 0x0F,

	ttyBold = 0x10,
	ttyUnderline = 0x20
};
struct styleTTY
{
	int v = 0;

	static bool s_ttySupportColour;
};
bool styleTTY::s_ttySupportColour = false;

std::ostream &operator<<( std::ostream &ostr, styleTTY i_style )
{
	if ( styleTTY::s_ttySupportColour )
	{
		if ( i_style.v != 0 )
		{
			std::string s( "\x1b[" );
			if ( i_style.v&ttyBold )
				s += "1;";
			if ( i_style.v&ttyUnderline )
				s += "4;";
			switch ( i_style.v&ttyColourMask )
			{
				case ttyRed: s += "31;"; break;
				case ttyGreen: s += "32;"; break;
				case ttyBlue: s += "34;"; break;
				case ttyCyan: s += "36;"; break;
				case ttyMagenta: s += "35;"; break;
				case ttyYellow: s += "33;"; break;
				case ttyBlack: s += "30;"; break;
				case ttyWhite: s += "37;"; break;
				default: break;
			}
			s.back() = 'm';
			ostr << s;
		}
		else
			ostr << "\x1b[0m";
	}
	return ostr;
}

//! nicer display name, for test suites and test cases
std::string displayName( const std::string &s )
{
	std::string result( s );
	// remove "timed_" if present
	if ( result.compare( 0, 6, "timed_" ) == 0 )
		result.erase( 0, 6 );
	// replate '_' with spaces
	std::replace( result.begin(), result.end(), '_', ' ' );
	return result;
}

//! all the test suites
std::vector<su::TestSuiteAbstract *> &getTestSuites()
{
	static std::vector<su::TestSuiteAbstract *> s_testSuites;
	return s_testSuites;
}

class SimpleTestDB
{
public:
	SimpleTestDB( const std::string_view &i_processName );
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

SimpleTestDB::SimpleTestDB( const std::string_view &i_processName )
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
		std::cerr << "ERROR: cannot find a path for the database" << std::endl;
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
		std::cerr << "ERROR: opening database failed: " << ex.what() << std::endl;
	}
	std::cout << styleTTY{ttyBold} << "Tests database: " << styleTTY{} << path << std::endl;
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
		std::cerr << "ERROR: inserting in database failed: " << ex.what() << std::endl;
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
		std::cerr << "ERROR: inserting in database failed: " << ex.what() << std::endl;
	}
#endif
	return std::numeric_limits<int64_t>::max();
}

}

namespace su {

void addTestSuite( TestSuiteAbstract *i_testsuite )
{
	getTestSuites().push_back( i_testsuite );
}

FailedTest::FailedTest( Type i_type, const tests_source_location &i_loc, const std::string_view &i_text, const std::string_view &i_msg )
	: _type( i_type ),
		_location( i_loc ),
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
		str << _location.file_name() << ":" << _location.function_name() << ":" << _location.line() << "\n";
		if ( not _msg.empty() )
			str << _msg << "\n";
		_storage = str.str();
	}
	return _storage.c_str();
}

int64_t TestTimer::nanoseconds()
{
	if ( _end < _start )
		end();
	return std::chrono::duration_cast<std::chrono::nanoseconds>(_end - _start).count();
}

}

int main( int argc, char **argv )
{
	//	remove the path base, just find the process name
	std::string_view processName{ argv[0] };
	auto pos = processName.find_last_of( "/\\" );
	if ( pos != std::string_view::npos )
		processName = processName.substr( pos+1 );
	
	enum class Action
	{
		kRunAll,
		kRunSome,
		kList
	};
	Action action = Action::kRunAll;
	std::set<std::string> testsToRun, testsToSkip;
	for ( int i = 1; i < argc; ++i )
	{
		// check for colour support
		if ( strcmp( argv[i], "--colour" ) == 0 )
			styleTTY::s_ttySupportColour = true;
		else if ( strcmp( argv[i], "--list" ) == 0 )
			action = Action::kList;
		else if ( strcmp( argv[i], "--run" ) == 0 and (i+1) < argc )
		{
			++i;
			testsToRun.emplace( argv[i] );
			action = Action::kRunSome;
		}
		else if ( strcmp( argv[i], "--skip" ) == 0 and (i+1) < argc )
		{
			++i;
			testsToSkip.emplace( argv[i] );
			action = Action::kRunSome;
		}
	}
	
	switch ( action )
	{
		case Action::kRunAll:
			std::cout << "Running all tests" << std::endl;
			break;
		case Action::kRunSome:
			std::cout << "Running selected tests" << std::endl;
			break;
		case Action::kList:
			std::cout << "Listing all tests" << std::endl;
			break;
	}
	std::unique_ptr<SimpleTestDB> db;
	if ( action != Action::kList )
	{
		// open db
		db = std::make_unique<SimpleTestDB>( processName );
	}
	
	// do all tests
	int total = 0;
	int failure = 0;
	for ( auto testSuite : getTestSuites() )
	{
		auto testSuiteDisplayName = displayName(testSuite->name());
		if ( action == Action::kRunSome )
		{
			if ( not testsToSkip.empty() )
			{
				if ( testsToSkip.find( testSuite->name() ) != testsToSkip.end() or
					testsToSkip.find( testSuiteDisplayName ) != testsToSkip.end() )
				{
					continue;
				}
			}
			else
			{
				if ( testsToRun.find( testSuite->name() ) == testsToRun.end() and
						testsToRun.find( testSuiteDisplayName ) == testsToRun.end() )
				{
					continue;
				}
			}
		}
		
		std::cout << styleTTY{ttyUnderline|ttyBold} << "Test suite:" << styleTTY{} << " "
					<< testSuiteDisplayName << std::endl;
		auto tests = testSuite->getTests();
		for ( auto test : tests )
		{
			std::cout << "  " << displayName(test.name());
			if ( action == Action::kList )
			{
				std::cout << std::endl;
				continue;
			}

			++total;

			std::cout << " : ";
			std::cout.flush();
			
			int64_t duration;
			std::string errorString;
			try
			{
				su::TestTimer timer;
				test( timer );
				duration = timer.nanoseconds();
				// test succeed, an exception would have occured otherwise
				const char kOK[] = "\xE2\x9C\x94";
				std::cout << styleTTY{ttyGreen|ttyBold} << kOK << styleTTY{};
				if ( test.timed() )
				{
					// a timed test, compare to last run
					auto prev = db->mostRecentDuration( testSuite->name(), test.name() );
					std::cout << " (" << duration << "ns";
					if ( duration > prev )
						std::cout << styleTTY{ttyRed} << " regression: " << duration - prev << "ns slower" << styleTTY{};
					std::cout << ")";
				}
			}
			catch ( su::FailedTest &ex )
			{
				errorString = std::string("TEST FAIL - ") + ex.what();
				++failure;
			}
			catch ( std::exception &ex )
			{
				errorString = std::string("EXCEPTION CAUGHT - ") + ex.what();
				++failure;
			}
			catch ( ... )
			{
				errorString = "UNKNOWN EXCEPTION CAUGHT";
				++failure;
			}
			if ( not errorString.empty() )
			{
				const char kFailed[] = "\xE2\x9C\x98";
				std::cout << styleTTY{ttyRed} << kFailed << " " << errorString << styleTTY{};
			}
			std::cout << std::endl;
			
			// record results
			db->addResult( testSuite->name(), test.name(), errorString, duration );
		}
	}
	if ( action != Action::kList )
	{
		std::cout << styleTTY{ttyBold} << "Success: " << styleTTY{} << (total-failure) << "/" << total << std::endl;
		if ( failure == 0 )
			std::cout << styleTTY{ttyGreen|ttyBold} << "All good!" << styleTTY{} << std::endl;
	}
	std::cout << std::endl;
}
