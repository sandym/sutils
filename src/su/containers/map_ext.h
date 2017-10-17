/*
 *  map_ext.h
 *  sutils
 *
 *  Created by Sandy Martel on 2016/09/23.
 *  Copyright (c) 2016年 Sandy Martel. All rights reserved.
 *
 * Permission to use, copy, modify, distribute, and sell this software for any
 * purpose is hereby granted without fee. The sotware is provided "AS-IS" and
 * without warranty of any kind, express, implied or otherwise.
 */

#ifndef H_SU_MAP
#define H_SU_MAP

#include <vector>

namespace su {

template<class MAP>
inline MAP::value_type value( const MAP &i_map, const MAP::key_type &k )
{
	auto it = i_map.find( k );
	if ( it != i_map.end() )
		return it->second;
	return {};
}

template<class MAP>
inline MAP::value_type value_or( const MAP &i_map, const MAP::key_type &k, const MAP::value_type &i_default )
{
	auto it = i_map.find( k );
	if ( it != i_map.end() )
		return it->second;
	return i_default;
}

template<class MAP>
inline MAP::key_type key( const MAP &i_map, const MAP::value_type &v )
{
	for ( auto &it : i_map )
	{
		if ( it.second == v )
			return it.first;
	}
	return {};
}

template<class MAP>
inline MAP::key_type key_or( const MAP &i_map, const MAP::value_type &v, const MAP::key_type &i_default  )
{
	for ( auto &it : i_map )
	{
		if ( it.second == v )
			return it.first;
	}
	return i_default;
}

template<class MAP>
inline MAP::value_type take( MAP &i_map, const MAP::key_type &k )
{
	auto it = i_map.find( k );
	if ( it != i_map.end() )
	{
		auto v = std::move(it->second);
		i_map.erase( it );
		return v;
	}
	return {};
}

template<class MAP>
inline MAP::value_type take_or( MAP &i_map, const MAP::key_type &k, const MAP::value_type &i_default )
{
	auto it = i_map.find( k );
	if ( it != i_map.end() )
	{
		auto v = std::move(it->second);
		i_map.erase( it );
		return v;
	}
	return i_default;
}

template<class MAP>
inline std::vector<MAP::key_type> keys( const MAP &i_map )
{
	std::vector<MAP::key_type> l;
	for ( auto &it : i_map )
		l.push_back( it.first );
	return l;
}

template<class MAP>
inline MAP merge( const MAP &lhs, const MAP &rhs )
{
	MAP m( lhs );
	for ( auto &it : rhs )
		m[it.first] = it.second;
	return m;
}

template<class MAP>
inline bool contains( const MAP &i_map, const MAP::key_type &k )
{
	return i_map.find( k ) != i_map.end();
}

}

#endif
