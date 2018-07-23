/*
 *  xml_tests.cpp
 *  sutils_tests
 *
 *  Created by Sandy Martel on 2015-08-30.
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
#include "su_saxparser.h"
#include "su_flat_map.h"

#ifdef HAS_EXPAT

#include <sstream>
#include <cctype>

class MyParser : public su::saxparser
{
public:
	MyParser( std::istream &i_stream )
		: su::saxparser( i_stream ){}

	virtual void startDocument()
	{
		++_seenStartDoc;
	}
	virtual void endDocument()
	{
		++_seenEndDoc;
	}
	virtual bool startElement( const su::NameURI &i_nameURI,
									const std::unordered_map<su::NameURI,std::string_view> &i_attribs )
	{
		_startElem.emplace_back( i_nameURI.name, i_nameURI.URI, i_attribs );
		return true;
	}
	virtual bool endElement( const su::NameURI &i_nameURI )
	{
		_endElem.emplace_back( i_nameURI.name, i_nameURI.URI );
		return true;
	}
	virtual bool characters( const std::string_view &i_s )
	{
		std::string_view s( i_s );
		while ( not s.empty() and std::isspace(s[0]) )
			s.remove_prefix( 1 );
		while ( not s.empty() and std::isspace(s.back()) )
			s.remove_suffix( 1 );
		
		if ( not s.empty() )
			_characters.push_back( std::string{ s } );
		return true;
	}

	int _seenStartDoc = 0;
	int _seenEndDoc = 0;

	struct Elem
	{
		Elem( const std::string_view &i_localname,
				const std::string_view &i_URI,
				const std::unordered_map<su::NameURI,std::string_view> &i_attribs )
			: localname( i_localname ),
				URI( i_URI )
		{
			for ( auto &it : i_attribs )
				attribs[MyNameURI{std::string(it.first.name),std::string(it.first.URI)}] = it.second;
		}
		Elem( const std::string_view &i_localname,
				const std::string_view &i_URI )
			: localname( i_localname ),
				URI( i_URI )
		{}
		
		std::string localname;
		std::string URI;
		struct MyNameURI
		{
			std::string name;
			std::string URI;
			bool operator<( const MyNameURI &rhs ) const { return std::tie(name,URI) < std::tie(rhs.name,rhs.URI); }
		};
		su::flat_map<MyNameURI,std::string> attribs;
	};
	std::vector<Elem> _startElem;
	std::vector<Elem> _endElem;
	std::vector<std::string> _characters;
};

struct xml_tests
{
	void test_case_1()
	{
		const char *xmlPtr = R"(<?xml version="1.0" encoding="UTF-8"?>
<instructionals module="Qt">
this is text
</instructionals>
)";
		
		std::istringstream istr( xmlPtr );
		MyParser parser( istr );
		
		parser.parse();
		
		TEST_ASSERT_EQUAL( parser._seenStartDoc, 1 );
		TEST_ASSERT_EQUAL( parser._seenEndDoc, 1 );
		TEST_ASSERT_EQUAL( parser._startElem.size(), 1 );
		TEST_ASSERT_EQUAL( parser._startElem.at(0).localname, "instructionals" );
		TEST_ASSERT( parser._startElem.at(0).URI.empty() );
		TEST_ASSERT_EQUAL( parser._startElem.at(0).attribs.size(), 1 );
		TEST_ASSERT_EQUAL( parser._startElem.at(0).attribs.begin()->first.name, "module" );
		TEST_ASSERT( parser._startElem.at(0).attribs.begin()->first.URI.empty() );
		TEST_ASSERT_EQUAL( parser._startElem.at(0).attribs.begin()->second, "Qt" );
		TEST_ASSERT_EQUAL( parser._endElem.size(), 1 );
		TEST_ASSERT_EQUAL( parser._endElem.at(0).localname, "instructionals" );
		TEST_ASSERT( parser._endElem.at(0).URI.empty() );

		TEST_ASSERT_EQUAL( parser._characters.size(), 1 );
		TEST_ASSERT_EQUAL( parser._characters.at(0), "this is text" );
	}
};

REGISTER_TEST_SUITE( xml_tests, su::timed_test(), &xml_tests::test_case_1 );

#endif
