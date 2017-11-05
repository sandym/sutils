/*
 *  abstractparser.cpp
 *  sutils
 *
 *  Created by Sandy Martel on 07-04-18.
 *  Copyright (c) 2015年 Sandy Martel. All rights reserved.
 *
 * Permission to use, copy, modify, distribute, and sell this software for any
 * purpose is hereby granted without fee. The sotware is provided "AS-IS" and
 * without warranty of any kind, express, implied or otherwise.
 */

#include "su/parsers/abstractparser.h"
#include "su/base/endian.h"
#include "su/strings/ConvertUTF.h"
#include <cctype>

namespace su {

abstractparserbase::abstractparserbase( std::istream &i_str, const std::string_view &i_name )
	: _stream( i_str ),
		_name( i_name )
{
	if ( not _stream )
		throw std::runtime_error( "invalid stream" );

	// auto detect format with the utf bom
	// need to be done without calling seekg so that it works on sequencial
	// stream...
	uint8_t c[3] = { 0, 0, 0 };
	_stream.read( (char *)c, 2 );
	if ( c[0] == 0xFE )
	{
		// might be UTF16-BE or UTF16-LE or invalid UTF8
		if ( c[1] == 0xFF )
		{
			_format = kutf16BE; // that's it we have a BOM -> FEFF
		}
		else if ( c[1] == 0 )
		{
			_format = kutf16LE;
			// hmm, weird FE00, just ignore it
		}
		else
		{
			_format = kutf8;
			putBackChar( c[1] );
			putBackChar( c[0] ); // hmm, weird UTF8
		}
	}
	else if ( c[0] == 0xFF )
	{
		// might be UTF16-LE or invalid UTF8
		if ( c[1] == 0xFE )
		{
			_format = kutf16LE; // that's it we have a BOM -> FFFE
		}
		else if ( c[1] == 0 )
		{
			_format = kutf16LE;
			// hmm, weird FF00, just ignore it
		}
		else
		{
			_format = kutf8;
			putBackChar( c[1] );
			putBackChar( c[0] ); // hmm, weird UTF8
		}
	}
	else if ( c[0] == 0xEF )
	{
		// might be UTF8 or invalid UTF8
		if ( c[1] == 0xBB )
		{
			_stream.read( (char *)&c+2, 1 );
			if ( c[2] == 0xBF )
			{
				_format = kutf8; // that's it we have a BOM -> EFBBBF
			}
			else
			{
				_format = kutf8;
				putBackChar( c[2] );
				putBackChar( c[1] );
				putBackChar( c[0] ); // hmm, weird UTF8
			}
		}
		else
		{
			_format = kutf8;
			putBackChar( c[1] );
			putBackChar( c[0] ); // hmm, weird UTF8
		}
	}
	else if ( c[0] == 0 )
	{
		_format = kutf16BE;
		putBackChar( c[1] ); // straight UTF16-BE
	}
	else
	{
		if ( c[1] == 0 )
		{
			_format = kutf16LE;
			putBackChar( c[0] ); // straight UTF16-LE
		}
		else
		{
			_format = kutf8;
			putBackChar( c[1] );
			putBackChar( c[0] ); // straight UTF8
		}
	}
}

char abstractparserbase::nextNonSpaceChar()
{
	char c;
	do
	{
		c = nextChar();
	}
	while ( c != 0 and isspace( c ) );
	return c;
}

char abstractparserbase::nextChar()
{
	// first look in the "put back" list
	if ( not _putbackCharList.empty() )
	{
		char c = _putbackCharList.top();
		_putbackCharList.pop();
		return c;
	}
	
	// is the stream still ok for reading?
	if ( not _stream )
		return 0;
	
	char c = 0;
	
	if ( _format == kutf8 )
	{
		_stream.read( (char *)&c, 1 );
		if ( _stream.gcount() != 1 )
			return 0;
	}
	else
	{
		// read a utf16 char and swap it in platfom native endian
		char16_t c16;
		_stream.read( (char *)&c16, 2 );
		if ( _stream.gcount() != 2 )
			return 0;
		if ( _format == kutf16BE )
			c16 = big_to_native<char16_t>( c16 );
		else
			c16 = little_to_native<char16_t>( c16 );
		
		auto sourceStart = (const UTF16 *)&c16;
		auto sourceEnd = sourceStart + 1;
		UTF8 buffer[6];
		UTF8 *targetStart = buffer;
		auto targetEnd = targetStart + 6;
		if ( ConvertUTF16toUTF8( &sourceStart, sourceEnd, &targetStart, targetEnd, lenientConversion ) == sourceIllegal )
			throw std::runtime_error( "invalid utf-16 file" );
		
		auto nb = targetStart - buffer;
		while ( nb > 1 )
			putBackChar( buffer[--nb] );
		c = buffer[0];
	}
	
	return c;
}

void abstractparserbase::putBackChar( char i_val )
{
	if ( i_val != 0 )
		_putbackCharList.push( i_val );
}

void abstractparserbase::skipToNextLine()
{
	char c;
	while ( (c = nextChar()) )
	{
		if ( c == '\n' )
			break; // \n
		if ( c == '\r' )
		{
			c = nextChar();
			if ( c != '\n' )
				putBackChar( c );
			break; // \r or \r\n
		}
	}
}

std::string abstractparserbase::readLine()
{
	std::string res;
	char c;
	while ( (c = nextChar()) )
	{
		if ( c == '\n' )
			break; // \n
		if ( c == '\r' )
		{
			c = nextChar();
			if ( c != '\n' )
				putBackChar( c );
			break; // \r or \r\n
		}
		res.push_back( c );
	}
	return res;
}

bool abstractparserbase::readDoubleQuotedString( std::string &o_s )
{
	o_s.clear();
	char c;
	if ( (c = nextNonSpaceChar()) )
	{
		if ( c == '\"' )
		{
			// read a quoted string
			while ( (c = nextChar()) )
			{
				if ( c == '\\' )
				{
					if ( (c = nextChar()) )
					{
						if ( c == 'n' or c == 'r' )
							o_s.append( 1, '\n' );
						else if ( c == 't' )
							o_s.append( 1, '\t' );
						else if ( c == 'b' )
							o_s.append( 1, '\b' );
						else if ( c == 'f' )
							o_s.append( 1, '\f' );
						else if ( c == '\"' )
							o_s.append( 1, '\"' );
						else if ( c == '\'' )
							o_s.append( 1, '\'' );
						else if ( c == '/' )
							o_s.append( 1, '/' );
						else
							o_s.append( 1, c );
					}
				}
				else if ( c == '\"' )
				{
					if ( (c = nextNonSpaceChar()) )
					{
						if ( c != '\"' )
						{
							putBackChar( c );
							break;
						}
					}
					else
						break;
				}
				else
					o_s.append( 1, c );
			}
			return true;
		}
		else
			putBackChar( c );
	}
	return false;
}

bool abstractparserbase::readNumber( std::string &o_s, bool &o_isFloat )
{
	o_isFloat = false;
	o_s.clear();
	char c;
	if ( (c = nextNonSpaceChar()) == 0 )
		return false;

	// sign
	char signChar = 0;
	if ( c == '+' or c == '-' )
	{
		signChar = c;
		if ( (c = nextChar()) == 0 )
		{
			putBackChar( signChar );
			return false;
		}
	}
	
	// int part
	if ( not std::isdigit( c ) )
	{
		putBackChar( c );
		putBackChar( signChar );
		return false;
	}
	
	if ( signChar != 0 )
		o_s.append( 1, signChar );
	
	while ( std::isdigit( c ) )
	{
		o_s.append( 1, c );
		c = nextChar();
	}
	
	if ( c == '.' )
	{
		o_isFloat = true;
		
		// point
		o_s.append( 1, c );
	
		c = nextChar();
	
		// decimal part
		while ( std::isdigit( c ) )
		{
			o_s.append( 1, c );
			c = nextChar();
		}
	}
	
	if ( c == 'e' or c == 'E' )
	{
		o_isFloat = true;

		// e
		o_s.append( 1, c );

		c = nextChar();
		if ( c == '+' or c == '-' )
		{
			o_s.append( 1, c );
			c = nextChar();
		}
		while ( std::isdigit( c ) )
		{
			o_s.append( 1, c );
			c = nextChar();
		}
	}
	putBackChar( c );
	return true;
}

void abstractparserbase::throwTokenizerException() const
{
	std::string ostr = "unexpected element in ";
	if ( _name.empty() )
		ostr += "stream";
	else
		ostr += _name;
	throw std::runtime_error( ostr );
}


#if 0
#pragma mark -
#endif

lineparser::lineparser( std::istream &i_str, const std::string_view &i_name )
	: abstractparser<linetoken_t>( i_str, i_name )
{
}

bool lineparser::parseAToken( token_type &o_token )
{
	o_token = token_type();
	std::string res;
	char c;
	while ( (c = nextChar()) )
	{
		switch ( c )
		{
			case '\n':
				o_token = token_type( line_tok_line, res );
				break; // \n
			case '\r':
				c = nextChar();
				if ( c != '\n' )
					putBackChar( c );
				o_token = token_type( line_tok_line, res );
				break; // \r or \r\n
			default:
				res.push_back( c );
				break;
		}
	}
	return o_token.isValid();
}

}
