/*
 *  saxparser.h
 *  sutils
 *
 *  Created by Sandy Martel on 08-09-18.
 *  Copyright (c) 2015年 Sandy Martel. All rights reserved.
 *
 * Permission to use, copy, modify, distribute, and sell this software for any
 * purpose is hereby granted without fee. The sotware is provided "AS-IS" and
 * without warranty of any kind, express, implied or otherwise.
 */

#ifndef H_SU_SAXPARSER
#define H_SU_SAXPARSER

#if __has_include(<expat.h>)
#define HAS_EXPAT

#include <unordered_map>
#include <string_view>
#include "su/containers/hash_combine.h"

namespace su {

struct NameURI
{
	std::string_view name;
	std::string_view URI;
	bool operator==( const NameURI &rhs ) const;
};

/*!
   @brief Simple SAX2 parser.
*/
class saxparser
{
public:
	saxparser( std::istream &i_stream );

	saxparser( const saxparser & ) = delete;
	saxparser &operator=( const saxparser & ) = delete;

	virtual ~saxparser() = default;
	
	virtual bool parse();
	
	virtual void startDocument();
	virtual void endDocument();
	virtual bool startElement( const NameURI &i_nameURI,
						const std::unordered_map<NameURI,std::string_view> &i_attribs );
	virtual bool endElement( const NameURI &i_nameURI );
	virtual bool characters( const std::string_view &i_text );
	
private:
	std::istream &_stream;
};

}

namespace std {

template<>
struct hash<su::NameURI>
{
	std::size_t operator()( const su::NameURI &k ) const
	{
		return su::hash_combine( k.name, k.URI );
	}
};

}

#endif

#endif
