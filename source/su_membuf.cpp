/*
 *  su_membuf.cpp
 *  sutils
 *
 *  Created by Sandy Martel on 2014/07/08.
 *  Copyright (c) 2015年 Sandy Martel. All rights reserved.
 *
 * Permission to use, copy, modify, distribute, and sell this software for any purpose is hereby
 * granted without fee. The sotware is provided "AS-IS" and without warranty of any kind, express,
 * implied or otherwise.
 */

#include "su_membuf.h"
#include <cassert>

namespace su
{

membuf::membuf( const char *i_begin, const char *i_end )
	: _begin( i_begin ),
		_end( i_end ),
		_current( i_begin )
{
	assert( _begin <= _end );
}

membuf::int_type membuf::uflow()
{
	if ( _current == _end )
		return traits_type::eof();

	return traits_type::to_int_type(*_current++);
}

membuf::int_type membuf::underflow()
{
	if ( _current == _end )
		return traits_type::eof();

	return traits_type::to_int_type(*_current);
}

membuf::int_type membuf::pbackfail( int_type ch )
{
	if ( _current == _begin or (ch != traits_type::eof() and ch != _current[-1]) )
		return traits_type::eof();

	return traits_type::to_int_type(*--_current);
}

std::streamsize membuf::showmanyc()
{
	assert( _current <= _end );
	return _end - _current;
}

}
