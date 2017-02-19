
#include "simple_tester.h"
#include <iostream>
#include <sstream>
#include <chrono>

namespace
{
std::vector<su::TestCaseAbstract *> *g_testCases;
}

namespace su
{

void addTestCase( TestCaseAbstract *i_tg )
{
	if ( g_testCases == nullptr )
		g_testCases = new std::vector<su::TestCaseAbstract *>;
	g_testCases->push_back( i_tg );
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
	for ( auto testcase : *g_testCases )
	{
		std::cout << "Test Case : " << testcase->name() << std::endl;
		auto tests = testcase->getTests();
		for ( auto test : tests )
		{
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
				std::cout << "OK (" << duration << ")";
			}
			catch ( su::FailedTest &ex )
			{
				result = "TEST FAIL - ";
				result += ex.what();
			}
			catch ( std::exception &ex )
			{
				result = "EXCEPTION CAUGHT - ";
				result += ex.what();
			}
			catch ( ... )
			{
				result = "UNKNOWN EXCEPTION CAUGHT";
			}
			std::cout << result << std::endl;
			
			//
			// addResultToDB( testcase->name(), test.name(), result, duration );
		}
	}
	std::cout << std::endl;
}
