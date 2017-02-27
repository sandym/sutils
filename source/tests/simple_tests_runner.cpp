
#include "tests/simple_tests.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <chrono>

namespace
{
std::vector<su::TestSuiteAbstract *> *g_testSuites;
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
	std::replace( _name.begin(), _name.end(), '_', ' ' );
}

TestSuiteAbstract::TestSuiteAbstract( const std::string &i_name )
	: _name( i_name )
{
	std::replace( _name.begin(), _name.end(), '_', ' ' );
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

int main()
{
	int total = 0;
	int failure = 0;
	for ( auto testSuite : *g_testSuites )
	{
		std::cout << "Test Case : " << testSuite->name() << std::endl;
		auto tests = testSuite->getTests();
		for ( auto test : tests )
		{
			++total;
			
			std::cout << "  " << test.name() << " : ";
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
					std::cout << " (" << duration << "ns)";
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
			
			//
			// addResultToDB( testSuite->name(), test.name(), result, duration );
		}
	}
	std::cout << "success: " << (total-failure) << "/" << total << std::endl;
	if ( failure == 0 )
		std::cout << "All good!" << std::endl;
	std::cout << std::endl;
}
