
#include "tests/simple_tests.h"
#include <ciso646>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <chrono>
#include <map>
#include <string.h>

#ifdef HAS_SQLITE3
#include <sqlite3.h>
#endif

namespace
{
std::vector<su::TestSuiteAbstract *> *g_testSuites;

std::string displayName( const std::string &s )
{
	std::string result( s );
	std::replace( result.begin(), result.end(), '_', ' ' );
	return result;
}

class SimpleTestDB
{
public:
	SimpleTestDB( const std::string &i_path, const std::string &i_processName );
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

SimpleTestDB::SimpleTestDB( const std::string &i_path, const std::string &i_processName )
	: _processName( i_processName )
{
#ifdef HAS_SQLITE3
	if ( i_path.empty() )
		return;
	
	try
	{
		int err = sqlite3_open( i_path.c_str(), &_db );
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

std::string getSimpleTestDBPath()
{
	std::string p;
	auto parent = getenv( "LOCALAPPDATA" );
	if ( parent == nullptr or *parent == 0 )
		parent = getenv( "HOME" );
	if ( parent != nullptr )
	{
		p.assign( parent );
		p += "/.simple_tests.db";
	}
	return p;
}

}

namespace su
{

TestCase::TestCase( const std::string &i_name, const func_t &i_test )
	: _name( i_name ), _test( i_test )
{
	if ( _name.compare( 0, 6, "timed_" ) == 0 )
	{
		_name.erase( 0, 6 );
		_timed = true;
	}
}

TestSuiteAbstract::TestSuiteAbstract( const std::string &i_name )
	: _name( i_name )
{
}

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
	
	SimpleTestDB db( getSimpleTestDBPath(), processName );
	
	int total = 0;
	int failure = 0;
	for ( auto testSuite : *g_testSuites )
	{
		std::cout << "Test Case : " << displayName(testSuite->name()) << std::endl;
		auto tests = testSuite->getTests();
		for ( auto test : tests )
		{
			++total;
			
			std::cout << "  " << displayName(test.name()) << " : ";
			std::cout.flush();
			
			std::string result;
			int64_t duration;
			try
			{
				auto start_time = std::chrono::high_resolution_clock::now();
				test();
				auto end_time = std::chrono::high_resolution_clock::now();
				duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time).count();
				std::cout << "OK";
				if ( test.timed() )
				{
					auto prev = db.mostRecentDuration( testSuite->name(), test.name() );
					std::cout << " (" << duration << "ns";
					if ( duration > prev )
						std::cout << " regression: " << duration - prev << "ns slower";
					std::cout << ")";
				}
			}
			catch ( su::FailedTest &ex )
			{
				result = "TEST FAIL - ";
				result += ex.what();
				++failure;
			}
			catch ( std::exception &ex )
			{
				result = "EXCEPTION CAUGHT - ";
				result += ex.what();
				++failure;
			}
			catch ( ... )
			{
				result = "UNKNOWN EXCEPTION CAUGHT";
				++failure;
			}
			std::cout << result << std::endl;
			
			db.addResult( testSuite->name(), test.name(), result, duration );
		}
	}
	std::cout << "success: " << (total-failure) << "/" << total << std::endl;
	if ( failure == 0 )
		std::cout << "All good!" << std::endl;
	std::cout << std::endl;
}
