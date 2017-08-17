/*
 *  filepath_tests.cpp
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
#include "su_filepath.h"
#include "su_logger.h"
#include <ciso646>

struct filepath_tests
{
	// declare all test cases here...
	void test_case_1();
	void test_case_2();
	void test_case_3();
	void test_case_4();
	void test_case_root();
	void test_case_6();
};

REGISTER_TEST_SUITE( filepath_tests,
					TEST_CASE(filepath_tests,test_case_1),
					TEST_CASE(filepath_tests,test_case_2),
					TEST_CASE(filepath_tests,test_case_3),
					TEST_CASE(filepath_tests,test_case_4),
					TEST_CASE(filepath_tests,test_case_root),
					TEST_CASE(filepath_tests,test_case_6) );

// MARK: -
// MARK:  === test cases ===

void filepath_tests::test_case_1()
{
	su::filepath fs;
	TEST_ASSERT( fs.empty() );

	su::filepath fs2( fs );
	TEST_ASSERT( fs2.empty() );
	
	fs = su::filepath( su::filepath::location::kHome );
	TEST_ASSERT( not fs.empty() );

	fs2 = fs;
	TEST_ASSERT( not fs2.empty() );
}

void filepath_tests::test_case_2()
{
	su::filepath fs;
	TEST_ASSERT( fs.empty() );

	fs = su::filepath( su::filepath::location::kHome );
	TEST_ASSERT( fs.isFolder() );
	TEST_ASSERT( fs.exists() );
	fs = su::filepath( su::filepath::location::kCurrentWorkingDir );
	TEST_ASSERT( fs.isFolder() );
	fs = su::filepath( su::filepath::location::kApplicationFolder );
	TEST_ASSERT( fs.isFolder() );
	fs = su::filepath( su::filepath::location::kApplicationSupport );
	TEST_ASSERT( fs.isFolder() );
	fs = su::filepath( su::filepath::location::kDesktop );
	TEST_ASSERT( fs.isFolder() );
	fs = su::filepath( su::filepath::location::kDocuments );
	TEST_ASSERT( fs.isFolder() );
	fs = su::filepath( su::filepath::location::kTempFolder );
	TEST_ASSERT( fs.isFolder() );
	fs = su::filepath( su::filepath::location::kNewTempSpec );
	TEST_ASSERT( not fs.isFolder() );
	TEST_ASSERT( not fs.isFile() );
	TEST_ASSERT( not fs.exists() );
	TEST_ASSERT( not fs.empty() );

	su::filepath fs2( su::filepath::location::kNewTempSpec );
	TEST_ASSERT( not fs2.isFolder() );
	TEST_ASSERT( not fs2.isFile() );
	TEST_ASSERT( not fs2.exists() );
	TEST_ASSERT( not fs2.empty() );
	TEST_ASSERT_NOT_EQUAL( fs2, fs );
}

void filepath_tests::test_case_3()
{
	su::filepath fs( su::filepath::location::kApplicationSupport );
	
	su::filepath::BookmarkData data;
	TEST_ASSERT( fs.getBookmarkData( data ) );
	
	su::filepath fs2( data );
	TEST_ASSERT_EQUAL( fs, fs2 );
	TEST_ASSERT_EQUAL( fs.path(), fs2.path() );
	TEST_ASSERT_EQUAL( fs.name(), fs2.name() );
}

void filepath_tests::test_case_4()
{
	su::filepath fs( su::filepath::location::kTempFolder );
	std::vector<su::filepath> list;
	
	TEST_ASSERT( fs.add( "123_testing.ext" ) );
	TEST_ASSERT( not fs.empty() );
	TEST_ASSERT( not fs.exists() );
	TEST_ASSERT_EQUAL( fs.name(), std::string_view("123_testing.ext") );
	TEST_ASSERT_EQUAL( fs.stem(), std::string_view("123_testing") );
	TEST_ASSERT_EQUAL( fs.extension(), std::string_view("ext") );
	fs.setExtension( "pdf" );
	TEST_ASSERT_EQUAL( fs.extension(), std::string_view("pdf") );
	
	fs.unlink();
	
	TEST_ASSERT( fs.mkdir() );
	auto mask = su::logger.getLogMask();
	su::logger.setLogMask( 0 );
	TEST_ASSERT( not fs.mkdir() );
	su::logger.setLogMask( mask );
	TEST_ASSERT( fs.isFolder() );
	TEST_ASSERT( fs.isFolderEmpty() );
	list = fs.folderContent();
	TEST_ASSERT( list.empty() );

	TEST_ASSERT( fs.unlink() );
	TEST_ASSERT( not fs.unlink() );
	TEST_ASSERT( not fs.isFolder() );
	TEST_ASSERT( not fs.exists() );
}

#if UPLATFORM_MAC
	std::string g_dataFolder( "/Users/Shared" );
	std::string g_dataFolderParent( "/Users" );
	std::string g_dataFolderParentParent( "/" );
#elif UPLATFORM_WIN
	std::string g_dataFolder(getenv("USERPROFILE"));
	std::string g_dataFolderParent("C:\\Users");
	std::string g_dataFolderParentParent("C:\\");
#else
	std::string g_dataFolder( getenv("HOME") );
	std::string g_dataFolderParent( "/home" );
	std::string g_dataFolderParentParent( "/" );
#endif

void filepath_tests::test_case_root()
{
	su::filepath fs( g_dataFolder );
	TEST_ASSERT( fs.isFolder() );
	TEST_ASSERT_EQUAL( fs.path(), g_dataFolder );
	fs.up();
	auto n = fs.name();
	TEST_ASSERT( fs.isFolder() );
	TEST_ASSERT_EQUAL( fs.path(), g_dataFolderParent );
	fs.up();
	TEST_ASSERT( fs.isFolder() );
	TEST_ASSERT_EQUAL( fs.path(), g_dataFolderParentParent );
	fs.add( n );
	TEST_ASSERT( fs.isFolder() );
	TEST_ASSERT_EQUAL( fs.path(), g_dataFolderParent );
}

void filepath_tests::test_case_6()
{
	su::filepath fs( g_dataFolder + "/titicata.test" );
	TEST_ASSERT( not fs.exists() );
	TEST_ASSERT( fs.mkdir() );
	TEST_ASSERT( fs.isFolder() );
	TEST_ASSERT( fs.move( su::filepath( g_dataFolder + "/totocaca.test" ) ) );
	TEST_ASSERT( not fs.exists() );
	fs = su::filepath( g_dataFolder + "/totocaca.test" );
	TEST_ASSERT( fs.isFolder() );
	fs.unlink();
	TEST_ASSERT( not fs.exists() );
}
