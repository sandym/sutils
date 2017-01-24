/*
 *  su_endian.h
 *  sutils
 *
 *  Created by Sandy Martel on 08-09-18.
 *  Copyright (c) 2015年 Sandy Martel. All rights reserved.
 *
 * Permission to use, copy, modify, distribute, and sell this software for any purpose is hereby
 * granted without fee. The sotware is provided "AS-IS" and without warranty of any kind, express,
 * implied or otherwise.
 */

#ifndef H_SU_ENDIAN
#define H_SU_ENDIAN

#include <type_traits>

namespace su
{

enum class endian
{
    little = 0,
    big    = 1,
#if defined(_WIN32) || defined(__LITTLE_ENDIAN__)
    native = little
#else
    native = big
#endif
};

namespace details
{
template<typename T,int S>
struct endian_swap { static T swap( T ); };

template<typename T>
struct endian_swap<T,1> { static T swap( T v ){ return v; } };

template<typename T>
struct endian_swap<T,2> { static T swap( T v )
{
	return ((((uint16_t)v)<<8)&0xFF00)|
			((((uint16_t)v)>>8)&0x00FF);
} };

template<typename T>
struct endian_swap<T,4> { static T swap( T v )
{
	return ((((uint32_t)v)<<24)&0xFF000000)|
			((((uint32_t)v)<<8)&0x00FF0000)|
			((((uint32_t)v)>>8)&0x0000FF00)|
			((((uint32_t)v)>>24)&0x000000FF);
} };

template<typename T>
struct endian_swap<T,8> { static T swap( T v )
{
	return ((((uint64_t)v)<<56)&0xFF00000000000000)|
			((((uint64_t)v)<<40)&0x00FF000000000000)|
			((((uint64_t)v)<<24)&0x0000FF0000000000)|
			((((uint64_t)v)<<8)&0x000000FF00000000)|
			((((uint64_t)v)>>8)&0x00000000FF000000)|
			((((uint64_t)v)>>24)&0x0000000000FF0000)|
			((((uint64_t)v)>>40)&0x000000000000FF00)|
			((((uint64_t)v)>>56)&0x00000000000000FF);
} };

}

// little
template<typename T>
std::enable_if_t<su::endian::native==su::endian::little,T>
inline little_to_native( T v ){ return v; }

template<typename T>
std::enable_if_t<su::endian::native==su::endian::little,T>
inline native_to_little( T v ){ return v; }

template<typename T>
std::enable_if_t<su::endian::native==su::endian::little,T>
inline big_to_native( T v )
{
	return details::endian_swap<T,sizeof(T)>::swap( v );
}

template<typename T>
std::enable_if_t<su::endian::native==su::endian::little,T>
inline native_to_big( T v )
{
	return details::endian_swap<T,sizeof(T)>::swap( v );
}

// big
template<typename T>
std::enable_if_t<su::endian::native==su::endian::big,T>
inline little_to_native( T v )
{
	return details::endian_swap<T,sizeof(T)>::swap( v );
}

template<typename T>
std::enable_if_t<su::endian::native==su::endian::big,T>
inline native_to_little( T v )
{
	return details::endian_swap<T,sizeof(T)>::swap( v );
}

template<typename T>
std::enable_if_t<su::endian::native==su::endian::big,T>
inline big_to_native( T v ){ return v; }

template<typename T>
std::enable_if_t<su::endian::native==su::endian::big,T>
inline native_to_big( T v ){ return v; }

}

#endif
