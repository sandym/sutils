/*
 *  su_teebuf.h
 *  sutils
 *
 *  Created by Sandy Martel on 2014/07/08.
 *  Copyright (c) 2015年 Sandy Martel. All rights reserved.
 *
 * Permission to use, copy, modify, distribute, and sell this software for any
 * purpose is hereby granted without fee. The sotware is provided "AS-IS" and
 * without warranty of any kind, express, implied or otherwise.
 */

#ifndef H_SU_TEEBUF
#define H_SU_TEEBUF

#include <ciso646>
#include <iostream>
#include <algorithm>

namespace su {

template<typename CHAR_TYPE, typename TRAITS = std::char_traits<CHAR_TYPE> >
class basic_teebuf : public std::basic_streambuf<CHAR_TYPE, TRAITS>
{
public:
	typedef typename TRAITS::int_type int_type;

	basic_teebuf( std::basic_streambuf<CHAR_TYPE, TRAITS> *sb1,
					std::basic_streambuf<CHAR_TYPE, TRAITS> *sb2 )
		: _sb1( sb1 ), _sb2( sb2 )
	{}

private:
	virtual int sync()
	{
		int r1 = _sb1->pubsync();
		int r2 = _sb2->pubsync();
		return r1 == 0 and r2 == 0 ? 0 : -1;
	}

	virtual std::streamsize xsputn( const CHAR_TYPE *s, std::streamsize n )
	{
		auto n1 = _sb1->sputn( s, n );
		auto n2 = _sb2->sputn( s, n );
		return std::max( n1, n2 );
	}

	virtual int_type overflow( int_type c )
	{
		int_type eof = TRAITS::eof();

		if (TRAITS::eq_int_type( c, eof ) )
		{
			return TRAITS::not_eof( c );
		}
		else
		{
			CHAR_TYPE ch = TRAITS::to_char_type( c );
			int_type r1 = _sb1->sputc( ch );
			int_type r2 = _sb2->sputc( ch );

			return TRAITS::eq_int_type( r1, eof ) or
					TRAITS::eq_int_type( r2, eof ) ? eof : c;
		}
	}

private:
	std::basic_streambuf<CHAR_TYPE, TRAITS> *_sb1;
	std::basic_streambuf<CHAR_TYPE, TRAITS> *_sb2;
};

typedef basic_teebuf<char> teebuf;

template<typename CHAR_TYPE, typename TRAITS = std::char_traits<CHAR_TYPE> >
class basic_teestream : private basic_teebuf<CHAR_TYPE, TRAITS>,
						public std::basic_ostream<CHAR_TYPE, TRAITS>
{
public:
	basic_teestream( std::basic_streambuf<CHAR_TYPE, TRAITS> *sb1,
						std::basic_streambuf<CHAR_TYPE, TRAITS> *sb2 )
		: basic_teebuf<CHAR_TYPE, TRAITS>( sb1, sb2 ),
			std::basic_ostream<CHAR_TYPE, TRAITS>( this )
	{
	}
};

typedef basic_teestream<char> teestream;

}

#endif
