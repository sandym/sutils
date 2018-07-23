/*
 *  binary_json_tests.cpp
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

#include "su_tests/simple_tests.h"
#include "su_json.h"
#include "su_resource_access.h"
#include <iostream>

struct binary_json_tests
{
	binary_json_tests();
	
	su::Json kTwitter;
	su::Json kCITM;
	su::Json kCanada;

	//	declare all test cases here...
	void test_case_bson();
	void test_case_ubjson();
	void test_case_smile();
	void test_case_messagepack();
	void test_case_flat();
};

REGISTER_TEST_SUITE( binary_json_tests,
			   su::timed_test(), &binary_json_tests::test_case_bson,
			   su::timed_test(), &binary_json_tests::test_case_ubjson,
			   su::timed_test(), &binary_json_tests::test_case_smile,
			   su::timed_test(), &binary_json_tests::test_case_messagepack,
			   su::timed_test(), &binary_json_tests::test_case_flat
			    );

namespace {
su::Json loadFile( const std::string &i_name )
{
	auto fpath = su::resource_access::get( i_name );
	
	std::ifstream f;
	fpath.fsopen( f );
	std::string s;
	while ( f )
	{
		char buf[4096];
		f.read( buf, 4096 );
		s.append( buf, f.gcount() );
	}
	std::string err;
	return su::Json::parse( s, err );
}
}

binary_json_tests::binary_json_tests()
{
	kCanada = loadFile( "canada.json" );
	kCITM = loadFile( "citm_catalog.json" );
	kTwitter = loadFile( "twitter.json" );
}

// MARK: -
// MARK:  === test cases ===

void binary_json_tests::test_case_bson()
{
	// TEST_ASSERT( not kCanada.is_null() );
	// auto data = su::bson::write( kCanada );
	// std::cout << data.size() << std::endl;
	// TEST_ASSERT( not data.empty() );
	// auto other = su::bson::read( { data.data(), data.size() } );
	// TEST_ASSERT_EQUAL( kCanada, other );
}

void binary_json_tests::test_case_ubjson()
{
	// TEST_ASSERT( not kCanada.is_null() );
	// auto data = su::ubjson::write( kCanada );
	// std::cout << data.size() << std::endl;
	// TEST_ASSERT( not data.empty() );
	// auto other = su::ubjson::read( data.data(), data.size() );
	// TEST_ASSERT_EQUAL( kCanada, other );
}

void binary_json_tests::test_case_smile()
{
	// TEST_ASSERT( not kCanada.is_null() );
	// auto data = su::smile::write( kCanada );
	// std::cout << data.size() << std::endl;
	// TEST_ASSERT( not data.empty() );
	// auto other = su::smile::read( data.data(), data.size() );
	// TEST_ASSERT_EQUAL( kCanada, other );
}

void binary_json_tests::test_case_messagepack()
{
	// TEST_ASSERT( not kCanada.is_null() );
	// auto data = su::messagepack::write( kCanada );
	// std::cout << data.size() << std::endl;
	// TEST_ASSERT( not data.empty() );
	// auto other = su::messagepack::read( data.data(), data.size() );
	// TEST_ASSERT_EQUAL( kCanada, other );
}

void binary_json_tests::test_case_flat()
{
	// TEST_ASSERT( not kCanada.is_null() );
	// auto data = su::flat::write( kCanada );
	// std::cout << data.size() << std::endl;
	// TEST_ASSERT( not data.empty() );
	// auto other = su::flat::read( data.data(), data.size() );
	// TEST_ASSERT_EQUAL( kCanada, other );
}
