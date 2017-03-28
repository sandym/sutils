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

#include <cassert>
#include <type_traits>
#include <string>
#include <functional>
#include <vector>
#include <cmath>
#include <chrono>

namespace su
{

/*!
	Simple nanoseconds timer.
*/
class TestTimer
{
public:
	//! record the start time
	inline void start()
	{
		_start = std::chrono::high_resolution_clock::now();
	}
	//! record the end time
	inline void end()
	{
		_end = std::chrono::high_resolution_clock::now();
	}
	
	//! return the time elapsed, ending the timer if needed
	int64_t nanoseconds();
	
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
	inline void operator()( TestTimer &i_timer )
	{
		_test( i_timer );
	}
	
	inline const std::string &name() const { return _name; }
	inline bool timed() const { return _name.compare( 0, 6, "timed_" ) == 0; }

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
	inline const std::string &name() const { return _name; }
	
	//! all test cases
	virtual std::vector<TestCase> getTests() = 0;
	
protected:
	TestSuiteAbstract( const std::string &i_name ) : _name( i_name ){}
	
	std::string _name;
};

//! record a new test suite
void addTestSuite( TestSuiteAbstract *i_testsuite );

//! get all recorded test suites
const std::vector<TestSuiteAbstract *> &getTestSuites();

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
			if ( it.methodWithTimer != nullptr )
			{
				// a method that handle timing
				auto methodPtr = it.methodWithTimer;
				result.emplace_back( it.name, [methodPtr]( TestTimer &i_timer )
					{
						T obj; // don't time setup / teardown
						i_timer.start(); // start in case the method forgot
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
						i_timer.start(); // start in case the function forgot
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
		
		// this should be a std::variant, only one of method,
		// methodWithTimer, func and funcWithTimer will be set
		// std::variant<(T::*)(),(T::*)(TestTimer&),std::function<void()>,std::function<void(TestTimer&)>> callback
		void (T::*method)() = nullptr;
		void (T::*methodWithTimer)(TestTimer&) = nullptr;
		const std::function<void()> func;
		const std::function<void(TestTimer&)> funcWithTimer;
	};
	std::vector<TestCaseData> _tests;
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

// helpers to raise exceptions
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

// small set of macros
#define REGISTER_TEST_SUITE(T,...) su::TestSuite<T> g_##T##_registration(#T,__VA_ARGS__)
#define TEST_CASE(C,N) #N,&C::N

#define TEST_FAIL(...) su::Fail(__FILE__,__LINE__, __VA_ARGS__ )
#define TEST_ASSERT(...) su::Assert(__FILE__,__LINE__, #__VA_ARGS__, __VA_ARGS__ )
#define TEST_ASSERT_EQUAL(...) su::Assert_equal( __FILE__,__LINE__, #__VA_ARGS__, __VA_ARGS__ )
#define TEST_ASSERT_NOT_EQUAL(...) su::Assert_not_equal( __FILE__,__LINE__, #__VA_ARGS__, __VA_ARGS__ )

#endif
