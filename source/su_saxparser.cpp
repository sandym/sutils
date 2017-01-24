/*
 *  su_saxparser.cpp
 *  sutils
 *
 *  Created by Sandy Martel on 08-09-18.
 *  Copyright (c) 2015年 Sandy Martel. All rights reserved.
 *
 * Permission to use, copy, modify, distribute, and sell this software for any purpose is hereby
 * granted without fee. The sotware is provided "AS-IS" and without warranty of any kind, express,
 * implied or otherwise.
 */

#include "su_saxparser.h"
#include <expat.h>
#include <cstring>
#include <iostream>

namespace
{

su::NameURI extractNameURI( const char *s )
{
	auto end = s + std::strlen( s );
	auto p = std::find( s, end, 0x0C );
	if ( p != end )
		return su::NameURI{ su::string_view( s, p-s ), su::string_view( p+1, end-(p+1) ) };
	else
		return su::NameURI{ su::string_view( s, end-s ) };
}

void startElement( void *userData, const char *name, const char **atts )
{
	XML_Parser parser = (XML_Parser)userData;
	su::saxparser *_this = (su::saxparser *)XML_GetUserData( parser );
	
	su::flat_map<su::NameURI,su::string_view> attribs;
	
	for ( int i = 0; atts[i] != nullptr; i += 2 )
	{
		attribs[extractNameURI(atts[i])] = atts[i+1];
	}

	auto n = extractNameURI( name );
	if ( not _this->startElement( n, attribs ) )
		XML_StopParser( parser, false );
}
void endElement( void *userData, const char *name )
{
	XML_Parser parser = (XML_Parser)userData;
	su::saxparser *_this = (su::saxparser *)XML_GetUserData( parser );

	auto n = extractNameURI( name );
	if ( not _this->endElement( n ) )
		XML_StopParser( parser, false );
}
void characters( void *userData, const char *s, int len )
{
	XML_Parser parser = (XML_Parser)userData;
	su::saxparser *_this = (su::saxparser *)XML_GetUserData( parser );

	if ( not _this->characters( su::string_view( s, len ) ) )
		XML_StopParser( parser, false );
}

}

namespace su
{

bool NameURI::operator<( const NameURI &rhs ) const
{
	return std::tie(name,URI) < std::tie(rhs.name,rhs.URI);
}

saxparser::saxparser( std::istream &i_stream )
	: _stream( i_stream )
{
}

saxparser::~saxparser()
{
}

bool saxparser::parse()
{
	auto parser = XML_ParserCreateNS( nullptr, 0x0C );

	XML_SetUserData( parser, this );
	XML_UseParserAsHandlerArg( parser );
	XML_SetElementHandler( parser, ::startElement, ::endElement );
	XML_SetCharacterDataHandler( parser, ::characters );

	bool ret = true;

	startDocument();

	int isFinal;
	do
	{
		char buf[4096];
		_stream.read( buf, 4096 );
		auto len = _stream.gcount();
		isFinal = _stream.eof() or len == 0;
		if ( XML_Parse( parser, buf, len, isFinal ) != XML_STATUS_OK )
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

}
