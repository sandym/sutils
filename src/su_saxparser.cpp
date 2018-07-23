/*
 *  su_saxparser.cpp
 *  sutils
 *
 *  Created by Sandy Martel on 08-09-18.
 *  Copyright (c) 2015年 Sandy Martel. All rights reserved.
 *
 * Permission to use, copy, modify, distribute, and sell this software for any
 * purpose is hereby granted without fee. The sotware is provided "AS-IS" and
 * without warranty of any kind, express, implied or otherwise.
 */

#include "su_saxparser.h"

#ifdef HAS_EXPAT
#include <expat.h>
#include <cstring>
#include <iostream>

namespace {

su::NameURI extractNameURI( const std::string_view &s )
{
	auto p = s.find( 0x0C );
	if ( p != std::string_view::npos )
		return su::NameURI{ s.substr( p+1 ), s.substr( 0, p ) };
	else
		return su::NameURI{ s, std::string_view() };
}

void startElement_cb( void *userData, const char *name, const char **atts )
{
	auto parser = static_cast<XML_Parser>( userData );
	auto _this = static_cast<su::saxparser *>( XML_GetUserData( parser ) );
	
	std::unordered_map<su::NameURI,std::string_view> attribs;
	
	for ( int i = 0; atts[i] != nullptr; i += 2 )
		attribs[extractNameURI(atts[i])] = atts[i+1];

	auto n = extractNameURI( name );
	if ( not _this->startElement( n, attribs ) )
		XML_StopParser( parser, false );
}
void endElement_cb( void *userData, const char *name )
{
	auto parser = static_cast<XML_Parser>( userData );
	auto _this = static_cast<su::saxparser *>( XML_GetUserData( parser ) );

	auto n = extractNameURI( name );
	if ( not _this->endElement( n ) )
		XML_StopParser( parser, false );
}
void characters_cb( void *userData, const char *s, int len )
{
	auto parser = static_cast<XML_Parser>( userData );
	auto _this = static_cast<su::saxparser *>( XML_GetUserData( parser ) );

	if ( not _this->characters( std::string_view( s, len ) ) )
		XML_StopParser( parser, false );
}

}

namespace su {

bool NameURI::operator==( const NameURI &rhs ) const
{
	return std::tie(name,URI) == std::tie(rhs.name,rhs.URI);
}

saxparser::saxparser( std::istream &i_stream )
	: _stream( i_stream )
{
}

bool saxparser::parse()
{
	auto parser = XML_ParserCreateNS( nullptr, 0x0C );

	XML_SetUserData( parser, this );
	XML_UseParserAsHandlerArg( parser );
	XML_SetElementHandler( parser, ::startElement_cb, ::endElement_cb );
	XML_SetCharacterDataHandler( parser, ::characters_cb );

	bool ret = true;

	startDocument();

	int isFinal;
	do
	{
		char buf[4096];
		_stream.read( buf, 4096 );
		auto len = _stream.gcount();
		isFinal = _stream.eof() or len == 0;
		if ( XML_Parse( parser, buf, (int)len, isFinal ) != XML_STATUS_OK )
		{
			ret = false;
			break;
		}
	}
	while ( not isFinal );

	endDocument();

	XML_ParserFree( parser );

	return ret;
}

void saxparser::startDocument()
{
}

void saxparser::endDocument()
{
}

bool saxparser::startElement( const NameURI &i_nameURI,
						const std::unordered_map<NameURI,std::string_view> &i_attribs )
{
	return true;
}

bool saxparser::endElement( const NameURI &i_nameURI )
{
	return true;
}

bool saxparser::characters( const std::string_view &i_text )
{
	return true;
}

}

#endif
