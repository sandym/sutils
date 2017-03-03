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

struct endian_tests
{
	//	declare all test cases here...
	void test_case_1byte();
	void test_case_2bytes();
	void test_case_4bytes();
	void test_case_8bytes();
};

REGISTER_TEST_SUITE( endian_tests,
	TEST_CASE(endian_tests,test_case_1byte),
	TEST_CASE(endian_tests,test_case_2bytes),
	TEST_CASE(endian_tests,test_case_4bytes),
	TEST_CASE(endian_tests,test_case_8bytes) );

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
	uint16_t v = 0x3362;
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
