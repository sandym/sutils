
#include "simple_tester.h"
#include <iostream>

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
			
			std::string results;
			try
			{
				test();
				std::cout << "OK";
			}
			catch ( std::exception &ex )
			{
				std::cout << "FAILED\n- \n";
				std::cout << ex.what();
				results = ex.what();
			}
			catch ( ... )
			{
				std::cout << "FAILED\n- unknown reason";
				results = "unknown reason";
			}
			
			std::cout << std::endl;
		}
	}
	std::cout << std::endl;
}
