/*
 *  xml_tests.cpp
 *  sutils_tests
 *
 *  Created by Sandy Martel on 2015-08-30.
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
#include "su_saxparser.h"
#include <sstream>

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
										const su::flat_map<su::NameURI,su::string_view> &i_attribs )
		{
			_startElem.emplace_back( i_nameURI.name, i_nameURI.URI, i_attribs );
			return true;
		}
		virtual bool endElement( const su::NameURI &i_nameURI )
		{
			_endElem.emplace_back( i_nameURI.name, i_nameURI.URI );
			return true;
		}
		virtual bool characters( const su::string_view &i_s )
		{
			su::string_view s( i_s );
			while ( not s.empty() and std::isspace(s[0]) )
				s.remove_prefix( 1 );
			while ( not s.empty() and std::isspace(s.back()) )
				s.remove_suffix( 1 );
			
			if ( not s.empty() )
				_characters.push_back( s.to_string() );
			return true;
		}
	
		int _seenStartDoc = 0;
		int _seenEndDoc = 0;
	
		struct Elem
		{
			Elem( const su::string_view &i_localname,
					const su::string_view &i_URI,
					const su::flat_map<su::NameURI,su::string_view> &i_attribs )
				: localname( i_localname.to_string() ),
					URI( i_URI.to_string() )
			{
				for ( auto &it : i_attribs )
					attribs[std::make_pair(it.first.name.to_string(),it.first.URI.to_string())] = it.second.to_string();
			}
			Elem( const su::string_view &i_localname,
					const su::string_view &i_URI )
				: localname( i_localname.to_string() ),
					URI( i_URI.to_string() )
			{}
			
			std::string localname;
			std::string URI;
			su::flat_map<std::pair<std::string,std::string>,std::string> attribs;
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
		TEST_ASSERT_EQUAL( parser._startElem.at(0).attribs.begin()->first.first, "module" );
		TEST_ASSERT( parser._startElem.at(0).attribs.begin()->first.second.empty() );
		TEST_ASSERT_EQUAL( parser._startElem.at(0).attribs.begin()->second, "Qt" );
		TEST_ASSERT_EQUAL( parser._endElem.size(), 1 );
		TEST_ASSERT_EQUAL( parser._endElem.at(0).localname, "instructionals" );
		TEST_ASSERT( parser._endElem.at(0).URI.empty() );

		TEST_ASSERT_EQUAL( parser._characters.size(), 1 );
		TEST_ASSERT_EQUAL( parser._characters.at(0), "this is text" );
	}
};

REGISTER_TESTS( xml_tests, TEST_CASE(xml_tests,test_case_1) );
