/*
 *  str_load.cpp
 *  sutils
 *
 *  Created by Sandy Martel on 06-03-21.
 *  Copyright (c) 2015年 Sandy Martel. All rights reserved.
 *
 * Permission to use, copy, modify, distribute, and sell this software for any
 * purpose is hereby granted without fee. The sotware is provided "AS-IS" and
 * without warranty of any kind, express, implied or otherwise.
 */

#include "su/strings/str_load.h"
#include "su/log/logger.h"
#include "su/strings/str_ext.h"
#include "su/base/platform.h"
#include <cassert>

#if (UPLATFORM_MAC || UPLATFORM_IOS)
#define USE_CF_IMPLEMENTAION 1
#include "su/base/cfauto.h"
#else
#define USE_CF_IMPLEMENTAION 0
#endif


#if !USE_CF_IMPLEMENTAION

#include "su/files/resource_access.h"
#include "su/parsers/abstractparser.h"
#include <unordered_map>

//! .strings file parsing and loading for non-Mac platform
namespace
{

//! the global localized string repository
using string_hash_map = std::unordered_map<std::string,std::string>;
std::unordered_map<std::string,string_hash_map> g_stringTable;

enum token_t
{
	tok_invalid,
	tok_string,  // string
	tok_equal,   // =
	tok_colon    // ;
};

/*!
 *  The parser class
 *      take a FILE* and break it into tokens (lexical analysis)
 *      skip comments
 */
class Tokenizer : public su::abstractparser<token_t>
{
public:
	Tokenizer( std::istream &i_str, const std::string_view &i_name )
		: su::abstractparser<token_t>( i_str, i_name )
	{}
	
protected:
	virtual bool parseAToken( su::token<token_t> &o_token );
};

/*!
 *  get the next lexical element from the stream
 *      return false if none found
 */
bool Tokenizer::parseAToken( su::token<token_t> &o_token )
{
	char aChar;
	do
	{
		// parse
		aChar = nextNonSpaceChar();
		if ( aChar != 0 )
		{
			if ( aChar == '/' )
			{
				aChar = nextChar();
				if ( aChar != 0 )
				{
					if ( aChar == '/' )
					{
						// a line comment
						skipToNextLine();
					}
					else if ( aChar == '*' )
					{
						// a delimited comment
						char prev = 0;
						while ( not (prev == '*' and aChar == '/') )
						{
							prev = aChar;
							if ( (aChar = nextChar()) == 0 )
								break;
						}
					}
					putBackChar( aChar );
				}
			}
			else if ( aChar == '=' )
			{
				o_token = token_type( tok_equal, "=" );
			}
			else if ( aChar == ';' )
			{
				o_token = token_type( tok_colon, ";" );
			}
			else if ( aChar == '\"' )
			{
				putBackChar( aChar );
				std::string s;
				if ( readDoubleQuotedString( s ) )
					o_token = token_type( tok_string, s );
				else
					throwTokenizerException();
			}
			else if ( isalnum( aChar ) or aChar == '_' )
			{
				std::string s( 1, aChar );
				while ( (aChar = nextChar()) != 0 )
				{
					if ( not isalnum( aChar ) and aChar != '_' )
						break;
					s.append( 1, aChar );
				}
				putBackChar( aChar );
				o_token = token_type( tok_string, s );
			}
			else
				throwTokenizerException();
		}
	}
	while ( not o_token.isValid() and aChar != 0  );
	return o_token.isValid();
}

/*!
 *  .strings file syntaxic parsing
 *      load the given string table in the localized string repository
 */
void loadStringTable( const std::string &i_table )
{
	string_hash_map table;
	try
	{
		std::ifstream file;
		su::resource_access::get( i_table + ".strings" ).fsopen( file );
		if ( file )
		{
			Tokenizer tokenizer( file, i_table );
			Tokenizer::token_type token;
			while ( tokenizer.nextToken( token ) )
			{
				if ( token.type() != tok_string )
					tokenizer.throwTokenizerException();
				std::string key = token.value();
				tokenizer.nextTokenForced( token, tok_equal );
				tokenizer.nextTokenForced( token, tok_string );
				table[key] = token.value();
				tokenizer.nextTokenForced( token, tok_colon );
			}
		}
	}
	catch ( std::exception &ex )
	{
		log_trace() << i_table << " : " << ex.what();
	}
	g_stringTable[i_table] = std::move(table);
}

}

#endif

#if 0
#pragma mark -
#endif

namespace su
{

/*!
 * load string from table
 */
std::string string_load( const std::string_view &i_key, const std::string_view &i_table )
{
#if USE_CF_IMPLEMENTAION
	// use CoreFoundation
	
	cfauto<CFStringRef> key( su::CreateCFString(i_key) );
	cfauto<CFStringRef> tableName( not i_table.empty() ? su::CreateCFString(i_table) : nullptr );
	cfauto<CFStringRef> resRef( CFBundleCopyLocalizedString( CFBundleGetMainBundle(), key, key, tableName ) );
	return su::to_string( resRef );

#else
	// use the code above
	std::string tableStr( i_table );
	auto table = g_stringTable.find( tableStr );
	if ( table == g_stringTable.end() )
		loadStringTable( tableStr );
	table = g_stringTable.find( tableStr );
	assert( table != g_stringTable.end() );
	auto it = table->second.find( std::string(i_key) );
	if ( it != table->second.end() )
		return it->second;
	else
	{
		log_debug() << "key not found " << i_key <<", " << i_table;
		return std::string(i_key);
	}
#endif
}

}
