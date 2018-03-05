/*
 *  hash_combine.h
 *  sutils
 *
 *  Created by Sandy Martel on 17/10/2017.
 *  Copyright (c) 2015年 Sandy Martel. All rights reserved.
 *
 * Permission to use, copy, modify, distribute, and sell this software for any
 * purpose is hereby granted without fee. The sotware is provided "AS-IS" and
 * without warranty of any kind, express, implied or otherwise.
 *
 */

#ifndef H_SU_HASH_COMBINE
#define H_SU_HASH_COMBINE

#include <functional>

namespace su {

inline std::size_t _hash_combine( std::size_t seed )
{
	return seed;
}
	
template<typename T, typename... R>
inline std::size_t _hash_combine( std::size_t seed, const T &v, R... r )
{
	std::hash<T> hasher;
	seed ^= hasher(v) + 0x9e3779b9 + (seed<<6) + (seed>>2);
	return _hash_combine( seed, r... );
}

template<typename... T>
inline std::size_t hash_combine( const T&... args )
{
	return _hash_combine( 0, args... );
}

}

namespace std {

template<typename T1,typename T2>
struct hash<std::pair<T1,T2>>
{
	std::size_t operator()( const std::pair<T1,T2> &k ) const
	{
		return su::hash_combine( k.first, k.second );
	}
};

}

#endif
