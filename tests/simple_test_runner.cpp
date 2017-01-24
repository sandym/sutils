
#include "simple_tester.h"
#include <iostream>

namespace
{
std::vector<su::TestCaseAbstract *> g_testCases;
}

namespace su
{

void addTestCase( TestCaseAbstract *i_tg )
{
	g_testCases.push_back( i_tg );
}

}

int main()
{
	for ( auto testcase : g_testCases )
	{
		std::cout << "Test Case : " << testcase->name() << std::endl;
		auto tests = testcase->getTests();
		for ( auto test : tests )
		{
			std::cout << "  " << test.name() << " : ";
			std::cout.flush();
			
			try
			{
				testcase->setup();
			}
			catch ( ... )
			{
				std::cout << "setup FAILED!" << std::endl;
				break;
			}
			
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
			
			try
			{
				testcase->teardown();
			}
			catch ( ... )
			{
				std::cout << " - teardown FAILED";
				break;
			}
			std::cout << std::endl;
		}
		
	}
}
