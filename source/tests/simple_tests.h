//
//  simple_tests.h
//  sutils
//
//  Created by Sandy Martel on 12/03/10.
//  Copyright (c) 2015年 Sandy Martel. All rights reserved.
//
// Permission to use, copy, modify, distribute, and sell this software for any purpose is hereby
// granted without fee. The sotware is provided "AS-IS" and without warranty of any kind, express,
// implied or otherwise.

#ifndef H_SIMPLE_TESTS
#define H_SIMPLE_TESTS

#ifdef ENABLE_SIMPLE_TESTS

#include <cassert>
#include <type_traits>
#include <string_view>
#include <functional>
#include <vector>
#include <cmath>
#include <chrono>

namespace su {

/*!
	Simple nanoseconds timer.
*/
class TestTimer
{
public:
	//! record the start time
	void start()
	{
		_start = std::chrono::high_resolution_clock::now();
	}
	//! record the end time
	void end()
	{
		_end = std::chrono::high_resolution_clock::now();
	}
	
	//! return the time elapsed, ending the timer if needed
	std::int64_t nanoseconds();
	
private:
	std::chrono::high_resolution_clock::time_point _start{};
	std::chrono::high_resolution_clock::time_point _end{};
};

/*!
	A single test
 		If name start with "timed_", time regression for the test
		will be tracked.
*/
class TestCase
{
public:
	TestCase( const std::string &i_name, const std::function<void(TestTimer&)> &i_test )
		: _name( i_name ), _test( i_test ){}

	//! run the test
	void operator()( TestTimer &i_timer )
	{
		_test( i_timer );
	}
	
	const std::string &name() const { return _name; }
	bool timed() const { return _name.compare( 0, 6, "timed_" ) == 0; }

private:
	std::string _name;
	std::function<void(TestTimer&)> _test;
};

//! Abstract base for a test suite
class TestSuiteAbstract
{
public:
	TestSuiteAbstract( const TestSuiteAbstract & ) = delete;
	TestSuiteAbstract &operator=( const TestSuiteAbstract & ) = delete;
	virtual ~TestSuiteAbstract() = default;
	
	//! name of the test suite
	const std::string &name() const { return _name; }
	
	//! all test cases
	virtual std::vector<TestCase> getTests() = 0;
	
protected:
	TestSuiteAbstract( const std::string &i_name ) : _name( i_name ){}
	
	std::string _name;
};

//! record a new test suite
void addTestSuite( TestSuiteAbstract *i_testsuite );

/*!
	A test suite
		record a list of test cases
*/
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
	
	// helpers to register test cases
	void registerTestCase( const std::string &i_name, void (T::*i_method)() )
	{
		_tests.push_back( TestCaseData{ i_name, i_method } );
	}
	void registerTestCase( const std::string &i_name, void (T::*i_method)(TestTimer&) )
	{
		_tests.push_back( TestCaseData{ i_name, i_method } );
	}
	void registerTestCase( const std::string &i_name, const std::function<void ()> &i_func )
	{
		_tests.push_back( TestCaseData{ i_name, i_func } );
	}
	void registerTestCase( const std::string &i_name, const std::function<void (TestTimer&)> &i_func )
	{
		_tests.push_back( TestCaseData{ i_name, i_func } );
	}
	void registerTestCase( const std::function<void (TestSuite<T>&)> &i_dynamicRegistry )
	{
		_dynamicRegistry = i_dynamicRegistry;
	}
	void registerTestCases(){}
	
	template<typename CB,typename ...ARGS>
	void registerTestCases( const std::string &i_name, const CB &cb, ARGS... args )
	{
		registerTestCase( i_name, cb );
		registerTestCases( args... );
	}
	template<typename ...ARGS>
	void registerTestCases( const std::function<void (TestSuite<T>&)> &i_dynamicRegistry, ARGS... args )
	{
		registerTestCase( i_dynamicRegistry );
		registerTestCases( args... );
	}
	
	virtual std::vector<TestCase> getTests()
	{
		if ( _dynamicRegistry )
		{
			// there are dynamic tests to be registered.
			_dynamicRegistry( *this );
			_dynamicRegistry = nullptr; // do it only once
		}
		
		std::vector<TestCase> result;
		result.reserve( _tests.size() );
		for ( auto it : _tests )
		{
			if ( it.method != nullptr )
			{
				// a method with external timer
				auto methodPtr = it.method;
				result.emplace_back( it.name, [methodPtr]( TestTimer &i_timer )
					{
						T obj; // don't time setup / teardown
						i_timer.start();
						(obj.*methodPtr)();
						i_timer.end();
					} );
			}
			else if ( it.methodWithTimer != nullptr )
			{
				// a method that handle timing
				auto methodPtr = it.methodWithTimer;
				result.emplace_back( it.name, [methodPtr]( TestTimer &i_timer )
					{
						T obj; // don't time setup / teardown
						i_timer.start(); // start in case the method forget
						(obj.*methodPtr)( i_timer );
						i_timer.nanoseconds(); // this will record the end only IF it wasn't already
					} );
			}
			else if ( it.func )
			{
				// a function with external timer
				auto func = it.func;
				result.emplace_back( it.name, [func]( TestTimer &i_timer )
					{
						i_timer.start();
						func();
						i_timer.end();
					} );
			}
			else if ( it.funcWithTimer )
			{
				// a function that handle timing
				auto func = it.funcWithTimer;
				result.emplace_back( it.name, [func]( TestTimer &i_timer )
					{
						i_timer.start(); // start in case the function forget
						func( i_timer );
						i_timer.nanoseconds(); // this will record the end only IF it wasn't already
					} );
			}
		}
		return result;
	}
	
private:
	std::function<void (TestSuite<T>&)> _dynamicRegistry; //!< to call to register test cases at runtime
	
	//! test case data, name and callback
	struct TestCaseData
	{
		TestCaseData( const std::string &i_name, void (T::*i_method)() )
			: name( i_name ), method( i_method ){}
		TestCaseData( const std::string &i_name, void (T::*i_method)(TestTimer&) )
			: name( i_name ), methodWithTimer( i_method ){}
		TestCaseData( const std::string &i_name, const std::function<void()> &i_func )
			: name( i_name ), func( i_func ){}
		TestCaseData( const std::string &i_name, const std::function<void(TestTimer&)> &i_func )
			: name( i_name ), funcWithTimer( i_func ){}
		
		std::string name;
		
		using method_t = void (T::*)();
		using methodWithTimer_t =  void (T::*)(TestTimer&);
		using func_t =  std::function<void()>;
		using funcWithTimer_t = std::function<void(TestTimer&)>;

		// this should be a std::variant, only one of method,
		// methodWithTimer, func and funcWithTimer will be set
		// std::variant<method_t,methodWithTimer_t,func_t,funcWithTimer_t> callback
		method_t method = nullptr;
		methodWithTimer_t methodWithTimer = nullptr;
		func_t func;
		funcWithTimer_t funcWithTimer;
	};
	std::vector<TestCaseData> _tests;
};

//! @todo: remove once in std
struct tests_source_location
{
	tests_source_location() = default;
	tests_source_location(const char *i_file, int i_line, const char *i_func)
		: _file(i_file), _line(i_line), _func(i_func) {}

	const char *_file = nullptr;
	int _line = -1;
	const char *_func = nullptr;

	const char *function_name() const { return _func; }
	const char *file_name() const { return _file; }
	int line() const { return _line; }
};

//! when a test fails
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
	FailedTest( Type i_type, const tests_source_location &i_loc, const std::string_view &i_text, const std::string_view &i_msg );
	
	virtual const char *what() const noexcept;

	const Type _type;
	tests_source_location _location;
	const std::string _text;
	const std::string _msg;

private:
	mutable std::string _storage;
};

// helpers to raise exceptions
inline void Fail( const tests_source_location &i_loc, const std::string_view &i_msg )
{
	throw FailedTest( FailedTest::Type::kFail, i_loc, {}, i_msg );
}
inline void Assert( const tests_source_location &i_loc, const std::string_view &i_text, bool i_result, const std::string_view &i_msg = {} )
{
	if ( i_result )
		return;
	throw FailedTest( FailedTest::Type::kAssert, i_loc, i_text, i_msg );
}

template<typename LHS, typename RHS>
inline void Assert_equal( const tests_source_location &i_loc, const std::string_view &i_text, const LHS &i_lhs, const RHS &i_rhs, const std::string_view &i_msg = {} )
{
	if ( i_lhs == i_rhs )
		return;
	throw FailedTest( FailedTest::Type::kEqual, i_loc, i_text, i_msg );
}
template<typename LHS, typename RHS,typename DELTA>
inline void Assert_equal( const tests_source_location &i_loc, const std::string_view &i_text, const LHS &i_lhs, const RHS &i_rhs, const DELTA &i_delta, const std::string_view &i_msg = {} )
{
	if ( std::abs( i_lhs - i_rhs ) < i_delta )
		return;
	throw FailedTest( FailedTest::Type::kEqual, i_loc, i_text, i_msg );
}

template<typename LHS, typename RHS>
inline void Assert_not_equal( const tests_source_location &i_loc, const std::string_view &i_text, const LHS &i_lhs, const RHS &i_rhs, const std::string_view &i_msg = {} )
{
	if ( i_lhs != i_rhs )
		return;
	throw FailedTest( FailedTest::Type::kNotEqual, i_loc, i_text, i_msg );
}
template<typename LHS, typename RHS,typename DELTA>
inline void Assert_not_equal( const tests_source_location &i_loc, const std::string_view &i_text, const LHS &i_lhs, const RHS &i_rhs, const DELTA &i_delta, const std::string_view &i_msg = {} )
{
	if ( std::abs( i_lhs - i_rhs ) > i_delta )
		return;
	throw FailedTest( FailedTest::Type::kNotEqual, i_loc, i_text, i_msg );
}

}

// small set of macros
#define REGISTER_TEST_SUITE(T,...) su::TestSuite<T> g_##T##_registration(#T,__VA_ARGS__)
#define TEST_CASE(C,N) #N,&C::N

#define TEST_FAIL(...) su::Fail({__FILE__,__LINE__,__FUNCTION__},__VA_ARGS__)
#define TEST_ASSERT(...) su::Assert({__FILE__,__LINE__,__FUNCTION__},#__VA_ARGS__,__VA_ARGS__)
#define TEST_ASSERT_EQUAL(...) su::Assert_equal({__FILE__,__LINE__,__FUNCTION__},#__VA_ARGS__,__VA_ARGS__)
#define TEST_ASSERT_NOT_EQUAL(...) su::Assert_not_equal({__FILE__,__LINE__,__FUNCTION__},#__VA_ARGS__,__VA_ARGS__)

#else

#include <cstdint>
#include <cassert>

// do nothing implementation for when ENABLE_SIMPLE_TESTS is not defined
struct TestTimer
{
	void start(){ assert( false ); }
	void end(){ assert( false ); }
	std::int64_t nanoseconds() { assert( false ); return 0; }
};

namespace su {
template<class T>
struct TestSuite
{
template<typename ...ARGS>
void registerTestCase( ARGS... args ){ assert( false ); }
};
}

#define REGISTER_TEST_SUITE(...)
#define TEST_FAIL(...)
#define TEST_ASSERT(...)
#define TEST_ASSERT_EQUAL(...)
#define TEST_ASSERT_NOT_EQUAL(...)

#endif

#endif
