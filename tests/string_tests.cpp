/*
 *  string_tests.cpp
 *  sutils_tests
 *
 *  Created by Sandy Martel on 2008/05/30.
 *  Copyright 2015 Sandy Martel. All rights reserved.
 *
 *  quick reference:
 *
 *      TEST_ASSERT( condition [, message] )
 *          Assertions that a condition is true.
 *
 *      TEST_FAIL( message )
 *          Fails with the specified message.
 *
 *      TEST_ASSERT_EQUAL( expected, actual [, message] )
 *          Asserts that two values are equals.
 *
 *      TEST_ASSERT_NOT_EQUAL( not expected, actual [, message] )
 *          Asserts that two values are NOT equals.
 *
 *      TEST_ASSERT_EQUAL( expected, actual [, delta, message] )
 *          Asserts that two values are equals.
 *
 *      TEST_ASSERT_NOT_EQUAL( not expected, actual [, delta, message] )
 *          Asserts that two values are NOT equals.
 *
 *      TEST_ASSERT_THROW( expression, ExceptionType [, message] )
 *          Asserts that the given expression throws an exception of the specified type.
 *
 *      TEST_ASSERT_NO_THROW( expression [, message] )
 *          Asserts that the given expression does not throw any exceptions.
 */

#include "simple_tester.h"
#include "su_string.h"

struct string_tests
{
	//	declare all test cases here...
	void test_case_convert();
	void test_case_upper_lower();
	void test_case_trimSpaces();
	void test_case_compare();
	void test_case_split_join();
	void test_case_japanese();
	void test_case_levenshteinDistance();
};

REGISTER_TESTS( string_tests,
				TEST_CASE(string_tests,test_case_convert),
				TEST_CASE(string_tests,test_case_upper_lower),
				TEST_CASE(string_tests,test_case_trimSpaces),
				TEST_CASE(string_tests,test_case_compare),
				TEST_CASE(string_tests,test_case_split_join),
				TEST_CASE(string_tests,test_case_japanese),
				TEST_CASE(string_tests,test_case_levenshteinDistance) );

// MARK: -
// MARK:  === test cases ===

void string_tests::test_case_convert()
{
	auto u16 = su::to_u16string( "aBcD" );
	
	TEST_ASSERT_EQUAL( "aBcD", su::to_string( L"aBcD" ) );
	TEST_ASSERT_EQUAL( "aBcD", su::to_string( u16 ) );
	TEST_ASSERT( L"aBcD" == su::to_wstring( "aBcD" ) );
	TEST_ASSERT( L"aBcD" == su::to_wstring( u16 ) );
	TEST_ASSERT( u16 == su::to_u16string( "aBcD" ) );
	TEST_ASSERT( u16 == su::to_u16string( L"aBcD" ) );

#if UPLATFORM_MAC || UPLATFORM_IOS
	TEST_ASSERT_EQUAL( "aBcD", su::to_string( CFSTR("aBcD") ) );
#endif
}

void string_tests::test_case_upper_lower()
{
	TEST_ASSERT_EQUAL( su::tolower("aAbBcC"), "aabbcc" );
	TEST_ASSERT_EQUAL( su::toupper("aAbBcC"), "AABBCC" );
}

void string_tests::test_case_trimSpaces()
{
	TEST_ASSERT_EQUAL( su::trimSpaces_view("aa"), "aa" );
	TEST_ASSERT_EQUAL( su::trimSpaces_view("aa "), "aa" );
	TEST_ASSERT_EQUAL( su::trimSpaces_view("aa  "), "aa" );
	TEST_ASSERT_EQUAL( su::trimSpaces_view("  aa"), "aa" );
	TEST_ASSERT_EQUAL( su::trimSpaces_view("  aa   "), "aa" );
}

void string_tests::test_case_compare()
{
	TEST_ASSERT_EQUAL( su::compare_nocase( "AaAa", "aAaA" ), 0 );
	TEST_ASSERT_EQUAL( su::ucompare_nocase( "AaAa", "aAaA" ), 0 );
	TEST_ASSERT( su::ucompare_nocase_numerically( "A1", "a5" ) < 0 );
	TEST_ASSERT( su::ucompare_nocase_numerically( "a5", "A10" ) < 0 );
}

void string_tests::test_case_split_join()
{
	std::string thisIsFourWords("this is four  words");
	auto res = su::split<char>( thisIsFourWords, ' ' );
	TEST_ASSERT_EQUAL( res.size(), size_t(4) );
	if ( res.size() == 4 )
	{
		TEST_ASSERT_EQUAL( res[0], "this" );
		TEST_ASSERT_EQUAL( res[1], "is" );
		TEST_ASSERT_EQUAL( res[2], "four" );
		TEST_ASSERT_EQUAL( res[3], "words" );
	}
	
	res = su::split_if<char>( thisIsFourWords, []( char c ){ return c == ' '; } );
	TEST_ASSERT_EQUAL( res.size(), size_t(4) );
	if ( res.size() == 4 )
	{
		TEST_ASSERT_EQUAL( res[0], std::string("this") );
		TEST_ASSERT_EQUAL( res[1], std::string("is") );
		TEST_ASSERT_EQUAL( res[2], std::string("four") );
		TEST_ASSERT_EQUAL( res[3], std::string("words") );
	}
}

void string_tests::test_case_japanese()
{
	const char kabc[] = "a b c";
	TEST_ASSERT_EQUAL( su::japaneseHiASCIIFix( kabc ), std::string("a b c") );
	const char16_t kabcH[] = { 0xFF41, 0x0020, 0xFF42, 0x0020, 0xFF43, 0 };
	TEST_ASSERT_EQUAL( su::japaneseHiASCIIFix( su::to_string( kabcH ) ), std::string("a b c") );

	const char knumber[] = "125";
	TEST_ASSERT_EQUAL( su::kanjiNumberFix( knumber ), std::string("125") );
	const char16_t kjnumber[] = { 0x767E, 0x4E8C, 0x5341, 0x4E94, 0 };
	TEST_ASSERT_EQUAL( su::kanjiNumberFix( su::to_string( kjnumber ) ), std::string("125") );
}

void string_tests::test_case_levenshteinDistance()
{
	TEST_ASSERT_EQUAL( su::levenshteinDistance( "ab", "cdefghij" ), size_t(8) );
	TEST_ASSERT_EQUAL( su::levenshteinDistance( "ab", "cd" ), size_t(2) );
	TEST_ASSERT_EQUAL( su::levenshteinDistance( "sandy", "sandym" ), size_t(1) );
	TEST_ASSERT_EQUAL( su::levenshteinDistance( "sandy", "sundy" ), size_t(1) );
	TEST_ASSERT_EQUAL( su::levenshteinDistance( "sandy", "sunday" ), size_t(2) );
	TEST_ASSERT_EQUAL( su::levenshteinDistance( "midori", "Midori" ), size_t(1) );
}