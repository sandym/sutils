/*
 *  teebuf.h
 *  sutils
 *
 *  Created by Sandy Martel on 2014/07/08.
 *  Copyright (c) 2015年 Sandy Martel. All rights reserved.
 *
 * Permission to use, copy, modify, distribute, and sell this software for any purpose is hereby
 * granted without fee. The sotware is provided "AS-IS" and without warranty of any kind, express,
 * implied or otherwise.
 */

#ifndef H_SU_TEEBUF
#define H_SU_TEEBUF

#include <iostream>
#include <ciso646>

namespace su
{

template<typename char_type, typename traits = std::char_traits<char_type> >
class basic_teebuf : public std::basic_streambuf<char_type, traits>
{
	public:
		typedef typename traits::int_type int_type;

		basic_teebuf( std::basic_streambuf<char_type, traits> *sb1, std::basic_streambuf<char_type, traits> *sb2 )
			: _sb1( sb1 ), _sb2( sb2 )
		{}

	private:    
		virtual int sync()
		{
			int r1 = _sb1->pubsync();
			int r2 = _sb2->pubsync();
			return r1 == 0 and r2 == 0 ? 0 : -1;
		}

		virtual std::streamsize xsputn( const char_type *s, std::streamsize n )
		{
			auto n1 = _sb1->sputn( s, n );
			auto n2 = _sb2->sputn( s, n );
			return std::min( n1, n2 );
		}
	
		virtual int_type overflow( int_type c )
		{
			int_type eof = traits::eof();

			if ( traits::eq_int_type( c, eof ) )
			{
				return traits::not_eof( c );
			}
			else
			{
				char_type ch = traits::to_char_type( c );
				int_type r1 = _sb1->sputc( ch );
				int_type r2 = _sb2->sputc( ch );
	
				return traits::eq_int_type( r1, eof ) or traits::eq_int_type( r2, eof ) ? eof : c;
			}
		}

	private:
		std::basic_streambuf<char_type, traits> *_sb1;
		std::basic_streambuf<char_type, traits> *_sb2;
};

typedef basic_teebuf<char> teebuf;

}

#endif
