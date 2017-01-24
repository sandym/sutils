
#ifndef H_TESTER
#define H_TESTER

#include <cassert>
#include <type_traits>
#include <string>
#include <functional>
#include <vector>

#define TEST_ASSERT( x ) assert( x )
#define TEST_ASSERT_EQUAL( a, b ) assert( a == b )
#define TEST_ASSERT_NOT_EQUAL( a, b ) assert( a != b )

namespace su
{

class SingleTest
{
	public:
		typedef std::function<void ()> func_t;
		
		SingleTest( const std::string &i_name, const func_t &i_test )
			: _name( i_name ), _test( i_test ){}
	
		//! run the test
		inline void operator()() { _test(); }
		
		inline const std::string &name() const { return _name; }
		inline bool isTest() const { return _name.find( "test_" ) == 0; }
		
	private:
		std::string _name;
		func_t _test;
};

class TestCaseAbstract
{
	public:
		TestCaseAbstract( const TestCaseAbstract & ) = delete;
		TestCaseAbstract &operator=( const TestCaseAbstract & ) = delete;
		virtual ~TestCaseAbstract() = default;
		
		inline const std::string &name() const { return _name; }
		
		virtual std::vector<SingleTest> getTests() = 0;
		
	protected:
		TestCaseAbstract( const std::string &i_name ) : _name( i_name ){}
		
		std::string _name;
};

void addTestCase( TestCaseAbstract *i_tg );
const std::vector<TestCaseAbstract *> &getTestSuite();

template<class T>
class TestCase : public TestCaseAbstract
{
	public:
		template<class ...ARGS>
		TestCase( const std::string &i_name, ARGS... args )
			: TestCaseAbstract( i_name )
		{
			registerTestCases( args... );
			addTestCase( this );
		}
		~TestCase() = default;
		void registerTestCases( void (T::*i_method)(), const std::string &i_name )
		{
			_tests.push_back( TestCaseData{ i_method, i_name } );
		}
		
		template<class ...ARGS>
		void registerTestCases( void (T::*i_method)(), const std::string &i_name, ARGS... args )
		{
			registerTestCases( i_method, i_name );
			registerTestCases( args... );
		}
		
		virtual std::vector<SingleTest> getTests()
		{
			std::vector<SingleTest> result;
			for ( auto it : _tests )
			{
				auto methodPtr = it.method;
				result.emplace_back( it.name, [methodPtr]()
					{
						T obj;
						(obj.*methodPtr)();
					} );
			}
			return result;
		}
		
	private:
		struct TestCaseData { void (T::*method)(); std::string name; };
		std::vector<TestCaseData> _tests;
};
}

#define REGISTER_TESTS(T,...) su::TestCase<T> g_##T##_registration(#T ,__VA_ARGS__)
#define TEST_CASE(C,N) &C::N, #N

#endif
