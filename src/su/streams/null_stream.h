/*
 *  null_stream.h
 *  sutils
 *
 *  Created by Sandy Martel on 2014/07/08.
 *  Copyright (c) 2017年 Sandy Martel. All rights reserved.
 *
 * Permission to use, copy, modify, distribute, and sell this software for any
 * purpose is hereby granted without fee. The sotware is provided "AS-IS" and
 * without warranty of any kind, express, implied or otherwise.
 */

#ifndef H_SU_NULL_STREAM
#define H_SU_NULL_STREAM

#include <iostream>

namespace su {

template<typename char_type, typename traits = std::char_traits<char_type> >
class basic_nullbuf : public std::basic_streambuf<char_type, traits>
{
public:
	typedef typename traits::int_type int_type;

	basic_nullbuf() = default;

private:    
	virtual int sync()
	{
		return 0;
	}

	virtual std::streamsize xsputn( const char_type *, std::streamsize n )
	{
		return n;
	}

	virtual int_type overflow( int_type c )
	{
		return c;
	}
};

template<typename char_type, typename traits = std::char_traits<char_type> >
class basic_null_stream : private basic_nullbuf<char_type,traits>, public std::ostream
{
public:
    basic_null_stream() : std::ostream( this ) {}
    basic_null_stream* rdbuf() const { return this; }
};

using null_stream = basic_null_stream<char>;

}

#endif
