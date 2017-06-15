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

namespace su {

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

namespace details {

template<typename T>
inline std::enable_if_t<sizeof(T)==1,T>
endian_swap( T v ) { return v; }

template<typename T>
inline std::enable_if_t<sizeof(T)==2,T>
endian_swap( T v )
{
	union {
		T v;
		uint16_t b;
	} u;
	u.v = v;
	u.b = ((u.b<<8)&0xFF00)|
			((u.b>>8)&0x00FF);
	return u.v;
}

template<typename T>
inline std::enable_if_t<sizeof(T)==4,T>
endian_swap( T v )
{
	union {
		T v;
		uint32_t b;
	} u;
	u.v = v;
	u.b = ((u.b<<24)&0xFF000000)|
			((u.b<<8)&0x00FF0000)|
			((u.b>>8)&0x0000FF00)|
			((u.b>>24)&0x000000FF);
	return u.v;
}

template<typename T>
inline std::enable_if_t<sizeof(T)==8,T>
endian_swap( T v )
{
	union {
		T v;
		uint64_t b;
	} u;
	u.v = v;
	u.b = ((u.b<<56)&0xFF00000000000000)|
			((u.b<<40)&0x00FF000000000000)|
			((u.b<<24)&0x0000FF0000000000)|
			((u.b<<8)&0x000000FF00000000)|
			((u.b>>8)&0x00000000FF000000)|
			((u.b>>24)&0x0000000000FF0000)|
			((u.b>>40)&0x000000000000FF00)|
			((u.b>>56)&0x00000000000000FF);
	return u.v;
}

//template<typename T>
//inline std::enable_if_t<sizeof(T)==16,T>
//endian_swap( T v )
//{
//	union {
//		T v;
//		struct { uint64_t b1, b2; } b;
//	} u;
//	u.v = v;
//	u.b.b1 = endian_swap( u.b.b1 );
//	u.b.b2 = endian_swap( u.b.b2 );
//	std::swap( u.b.b1, u.b.b2 );
//	return u.v;
//}

}

// little
template<typename T>
inline std::enable_if_t<su::endian::native==su::endian::little,T>
little_to_native( T v ){ return v; }

template<typename T>
inline std::enable_if_t<su::endian::native==su::endian::little,T>
native_to_little( T v ){ return v; }

template<typename T>
inline std::enable_if_t<su::endian::native==su::endian::little,T>
big_to_native( T v )
{
	return details::endian_swap( v );
}

template<typename T>
inline std::enable_if_t<su::endian::native==su::endian::little,T>
native_to_big( T v )
{
	return details::endian_swap( v );
}

// big
template<typename T>
inline std::enable_if_t<su::endian::native==su::endian::big,T>
little_to_native( T v )
{
	return details::endian_swap( v );
}

template<typename T>
inline std::enable_if_t<su::endian::native==su::endian::big,T>
native_to_little( T v )
{
	return details::endian_swap( v );
}

template<typename T>
inline std::enable_if_t<su::endian::native==su::endian::big,T>
big_to_native( T v ){ return v; }

template<typename T>
inline std::enable_if_t<su::endian::native==su::endian::big,T>
native_to_big( T v ){ return v; }

}

#endif
