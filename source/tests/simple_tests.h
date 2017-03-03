
#ifndef H_TESTER
#define H_TESTER

#include <cassert>
#include <type_traits>
#include <string>
#include <functional>
#include <vector>
#include <cmath>

namespace su
{

class TestCase
{
	public:
		typedef std::function<void ()> func_t;
		
		TestCase( const std::string &i_name, const func_t &i_test );
	
		//! run the test
		inline void operator()() { _test(); }
		
		inline const std::string &name() const { return _name; }
		inline bool timed() const { return _timed; }
	
	private:
		std::string _name;
		bool _timed = false;
		func_t _test;
};

class TestSuiteAbstract
{
	public:
		TestSuiteAbstract( const TestSuiteAbstract & ) = delete;
		TestSuiteAbstract &operator=( const TestSuiteAbstract & ) = delete;
		virtual ~TestSuiteAbstract() = default;
		
		inline const std::string &name() const { return _name; }
		
		virtual std::vector<TestCase> getTests() = 0;
		
	protected:
		TestSuiteAbstract( const std::string &i_name );
		
		std::string _name;
};

void addTestSuite( TestSuiteAbstract *i_tg );
const std::vector<TestSuiteAbstract *> &getTestSuite();

template<class T>
class TestSuite : public TestSuiteAbstract
{
public:
	template<class ...ARGS>
	TestSuite( const std::string &i_name, ARGS... args )
		: TestSuiteAbstract( i_name )
	{
		registerTestCases( args... );
		addTestSuite( this );
	}
	~TestSuite() = default;
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
	
	template<typename FUNC>
	void registerTestCases( const FUNC &i_dynamicRegistry )
	{
		_dynamicRegistry = i_dynamicRegistry;
	}
	
	virtual std::vector<TestCase> getTests()
	{
		if ( _dynamicRegistry )
		{
			_dynamicRegistry( *this );
			_dynamicRegistry = nullptr;
		}
		
		std::vector<TestCase> result;
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
	std::function<void (TestSuite<T>&)> _dynamicRegistry;
	struct TestCaseData { void (T::*method)(); std::string name; };
	std::vector<TestCaseData> _tests;
};

class FailedTest : public std::exception
{
public:
	enum class Type
	{
		kFail,
		kAssert,
		kEqual,
		kNotEqual
	};
	FailedTest( Type i_type, const char *i_file, int i_line, const std::string &i_text, const std::string &i_msg );
	
	virtual const char *what() const noexcept;

	const Type _type;
	const char *_file;
	const int _line;
	const std::string _text;
	const std::string _msg;

private:
	mutable std::string _storage;
};

inline void Fail( const char *i_file, int i_line, const std::string &i_msg )
{
	throw FailedTest( FailedTest::Type::kFail, i_file, i_line, {}, i_msg );
}
inline void Assert( const char *i_file, int i_line, const std::string &i_text, bool i_result, const std::string &i_msg = {} )
{
	if ( i_result )
		return;
	throw FailedTest( FailedTest::Type::kAssert, i_file, i_line, i_text, i_msg );
}

template<typename LHS, typename RHS>
inline void Assert_equal( const char *i_file, int i_line, const std::string &i_text, const LHS &i_lhs, const RHS &i_rhs, const std::string &i_msg = {} )
{
	if ( i_lhs == i_rhs )
		return;
	throw FailedTest( FailedTest::Type::kEqual, i_file, i_line, i_text, i_msg );
}
template<typename LHS, typename RHS,typename DELTA>
inline void Assert_equal( const char *i_file, int i_line, const std::string &i_text, const LHS &i_lhs, const RHS &i_rhs, const DELTA &i_delta, const std::string &i_msg = {} )
{
	if ( std::abs( i_lhs - i_rhs ) < i_delta )
		return;
	throw FailedTest( FailedTest::Type::kEqual, i_file, i_line, i_text, i_msg );
}

template<typename LHS, typename RHS>
inline void Assert_not_equal( const char *i_file, int i_line, const std::string &i_text, const LHS &i_lhs, const RHS &i_rhs, const std::string &i_msg = {} )
{
	if ( i_lhs != i_rhs )
		return;
	throw FailedTest( FailedTest::Type::kNotEqual, i_file, i_line, i_text, i_msg );
}
template<typename LHS, typename RHS,typename DELTA>
inline void Assert_not_equal( const char *i_file, int i_line, const std::string &i_text, const LHS &i_lhs, const RHS &i_rhs, const DELTA &i_delta, const std::string &i_msg = {} )
{
	if ( std::abs( i_lhs - i_rhs ) > i_delta )
		return;
	throw FailedTest( FailedTest::Type::kNotEqual, i_file, i_line, i_text, i_msg );
}

}

#define REGISTER_TEST_SUITE(T,...) su::TestSuite<T> g_##T##_registration(#T,__VA_ARGS__)
#define TEST_CASE(C,N) &C::N,#N

#define TEST_FAIL(...) su::Fail(__FILE__,__LINE__, __VA_ARGS__ )
#define TEST_ASSERT(...) su::Assert(__FILE__,__LINE__, #__VA_ARGS__, __VA_ARGS__ )
#define TEST_ASSERT_EQUAL(...) su::Assert_equal( __FILE__,__LINE__, #__VA_ARGS__, __VA_ARGS__ )
#define TEST_ASSERT_NOT_EQUAL(...) su::Assert_not_equal( __FILE__,__LINE__, #__VA_ARGS__, __VA_ARGS__ )

#endif
