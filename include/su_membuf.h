/*
 *  su_membuf.h
 *  sutils
 *
 *  Created by Sandy Martel on 2014/07/08.
 *  Copyright (c) 2015年 Sandy Martel. All rights reserved.
 *
 * Permission to use, copy, modify, distribute, and sell this software for any purpose is hereby
 * granted without fee. The sotware is provided "AS-IS" and without warranty of any kind, express,
 * implied or otherwise.
 */

#ifndef H_SU_MEMBUF
#define H_SU_MEMBUF

#include <streambuf>

namespace su
{

class membuf : public std::streambuf
{
	public:
		membuf( const char *i_begin, const char *i_end );
		virtual ~membuf() = default;
	
	private:
		// Locales:
		//virtual void imbue( const locale& loc );
	
		// Buffer management and positioning:
//		virtual basic_streambuf *setbuf( char_type *s, std::streamsize n );
//		virtual pos_type seekoff( off_type off, std::ios_base::seekdir way,
//									std::ios_base::openmode which = std::ios_base::in | std::ios_base::out );
//		virtual pos_type seekpos( pos_type sp,
//									std::ios_base::openmode which = std::ios_base::in | std::ios_base::out );
//		virtual int sync();

		// Get area:
		virtual std::streamsize showmanyc();
//		virtual std::streamsize xsgetn( char_type *s, std::streamsize n );
		virtual int_type underflow();
		virtual int_type uflow();

		// Putback:
		virtual int_type pbackfail( int_type c = traits_type::eof() );

		// Put area:
//		virtual std::streamsize xsputn( const char_type *s, std::streamsize n );
//		virtual int_type overflow( int_type c = traits_type::eof() );

		membuf( const membuf & ) = delete;
		membuf &operator=( const membuf & ) = delete;

		const char * const _begin;
		const char * const _end;
		const char *_current;
};

}

#endif
