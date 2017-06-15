/*
 *  endian_tests.cpp
 *  sutils_tests
 *
 *  Created by Sandy Martel on 2008/05/30.
 *  Copyright 2015 Sandy Martel. All rights reserved.
 *
 *  quick reference:
 *
 *      TEST_ASSERT( condition )
 *          Assertions that a condition is true.
 *
 *      TEST_ASSERT_EQUAL( expected, actual )
 *          Asserts that two values are equals.
 *
 *      TEST_ASSERT_NOT_EQUAL( not expected, actual )
 *          Asserts that two values are NOT equals.
 */

#include "tests/simple_tests.h"
#include "su_endian.h"
#include <iostream>

struct endian_tests
{
	//	declare all test cases here...
	void test_case_1byte();
	void test_case_2bytes();
	void test_case_4bytes();
	void test_case_8bytes();
	void test_case_float();
	void test_case_double();
	void test_case_long_double();
};

REGISTER_TEST_SUITE( endian_tests,
	TEST_CASE(endian_tests,test_case_1byte),
	TEST_CASE(endian_tests,test_case_2bytes),
	TEST_CASE(endian_tests,test_case_4bytes),
	TEST_CASE(endian_tests,test_case_8bytes),
	TEST_CASE(endian_tests,test_case_float),
	TEST_CASE(endian_tests,test_case_double) );

// MARK: -
// MARK:  === test cases ===

void endian_tests::test_case_1byte()
{
	uint8_t	v = 0x55;
	TEST_ASSERT_EQUAL( su::little_to_native( v ), uint8_t(0x55) );
	TEST_ASSERT_EQUAL( su::native_to_little( v ), uint8_t(0x55) );
	TEST_ASSERT_EQUAL( su::big_to_native( v ), uint8_t(0x55) );
	TEST_ASSERT_EQUAL( su::native_to_big( v ), uint8_t(0x55) );
}

void endian_tests::test_case_2bytes()
{
	char16_t v = 0x3362;
	if ( su::endian::native == su::endian::big )
	{
		TEST_ASSERT_EQUAL( su::little_to_native( v ), uint16_t(0x6233) );
		TEST_ASSERT_EQUAL( su::native_to_little( v ), uint16_t(0x6233) );
		TEST_ASSERT_EQUAL( su::big_to_native( v ), uint16_t(0x3362) );
		TEST_ASSERT_EQUAL( su::native_to_big( v ), uint16_t(0x3362) );
	}
	else
	{
		TEST_ASSERT_EQUAL( su::little_to_native( v ), uint16_t(0x3362) );
		TEST_ASSERT_EQUAL( su::native_to_little( v ), uint16_t(0x3362) );
		TEST_ASSERT_EQUAL( su::big_to_native( v ), uint16_t(0x6233) );
		TEST_ASSERT_EQUAL( su::native_to_big( v ), uint16_t(0x6233) );
	}
}

void endian_tests::test_case_4bytes()
{
	uint32_t v = 0x01020304;
	if ( su::endian::native == su::endian::big )
	{
		TEST_ASSERT_EQUAL( su::little_to_native( v ), uint32_t(0x04030201) );
		TEST_ASSERT_EQUAL( su::native_to_little( v ), uint32_t(0x04030201) );
		TEST_ASSERT_EQUAL( su::big_to_native( v ), uint32_t(0x01020304) );
		TEST_ASSERT_EQUAL( su::native_to_big( v ), uint32_t(0x01020304) );
	}
	else
	{
		TEST_ASSERT_EQUAL( su::little_to_native( v ), uint32_t(0x01020304) );
		TEST_ASSERT_EQUAL( su::native_to_little( v ), uint32_t(0x01020304) );
		TEST_ASSERT_EQUAL( su::big_to_native( v ), uint32_t(0x04030201) );
		TEST_ASSERT_EQUAL( su::native_to_big( v ), uint32_t(0x04030201) );
	}
}

void endian_tests::test_case_8bytes()
{
	uint64_t v = 0x0102030405060708;
	if ( su::endian::native == su::endian::big )
	{
		TEST_ASSERT_EQUAL( su::little_to_native( v ), uint64_t(0x0807060504030201) );
		TEST_ASSERT_EQUAL( su::native_to_little( v ), uint64_t(0x0807060504030201) );
		TEST_ASSERT_EQUAL( su::big_to_native( v ), uint64_t(0x0102030405060708) );
		TEST_ASSERT_EQUAL( su::native_to_big( v ), uint64_t(0x0102030405060708) );
	}
	else
	{
		TEST_ASSERT_EQUAL( su::little_to_native( v ), uint64_t(0x0102030405060708) );
		TEST_ASSERT_EQUAL( su::native_to_little( v ), uint64_t(0x0102030405060708) );
		TEST_ASSERT_EQUAL( su::big_to_native( v ), uint64_t(0x0807060504030201) );
		TEST_ASSERT_EQUAL( su::native_to_big( v ), uint64_t(0x0807060504030201) );
	}
}

void endian_tests::test_case_float()
{
	float v = 1.234567f;
	if ( su::endian::native == su::endian::big )
	{
		auto s = su::native_to_big( v );
		TEST_ASSERT_EQUAL( v, s );
		s = su::native_to_little( v );
		s = su::little_to_native( s );
		TEST_ASSERT_EQUAL( v, s );
	}
	else
	{
		auto s = su::native_to_little( v );
		TEST_ASSERT_EQUAL( v, s );
		s = su::native_to_big( v );
		s = su::big_to_native( s );
		TEST_ASSERT_EQUAL( v, s );
	}
}

void endian_tests::test_case_double()
{
	double v = 1.234567;
	if ( su::endian::native == su::endian::big )
	{
		auto s = su::native_to_big( v );
		TEST_ASSERT_EQUAL( v, s );
		s = su::native_to_little( v );
		s = su::little_to_native( s );
		TEST_ASSERT_EQUAL( v, s );
	}
	else
	{
		auto s = su::native_to_little( v );
		TEST_ASSERT_EQUAL( v, s );
		s = su::native_to_big( v );
		s = su::big_to_native( s );
		TEST_ASSERT_EQUAL( v, s );
	}
}

void endian_tests::test_case_long_double()
{
//	long double v = 1.234567;
//	if ( su::endian::native == su::endian::big )
//	{
//		auto s = su::native_to_big( v );
//		TEST_ASSERT_EQUAL( v, s );
//		s = su::native_to_little( v );
//		s = su::little_to_native( s );
//		TEST_ASSERT_EQUAL( v, s );
//	}
//	else
//	{
//		auto s = su::native_to_little( v );
//		TEST_ASSERT_EQUAL( v, s );
//		s = su::native_to_big( v );
//		s = su::big_to_native( s );
//		TEST_ASSERT_EQUAL( v, s );
//	}
}
