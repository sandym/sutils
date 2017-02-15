/*
 *  su_flat_map.h
 *  sutils
 *
 *  Created by Sandy Martel on 22/11/2011.
 *  Copyright (c) 2015年 Sandy Martel. All rights reserved.
 *
 * Permission to use, copy, modify, distribute, and sell this software for any purpose is hereby
 * granted without fee. The sotware is provided "AS-IS" and without warranty of any kind, express,
 * implied or otherwise.
 *
 */

#ifndef H_SU_FLAT_MAP
#define H_SU_FLAT_MAP

#include <vector>
#include <algorithm>
#include <functional>
#include <ciso646>

namespace su
{
/*!
	A map implemented with a vector for better cache coherency
*/
template <class Key, class T, class CMP = std::less<Key>>
class flat_map
{
  public:
	// types:
	using vector_type = std::vector<std::pair<Key, T>>;
	using key_type = Key;
	using mapped_type = T;
	using value_type = typename vector_type::value_type;
	using reference = typename vector_type::reference;
	using const_reference = typename vector_type::const_reference;
	using pointer = typename vector_type::pointer;
	using const_pointer = typename vector_type::const_pointer;
	using size_type = typename vector_type::size_type;
	using difference_type = typename vector_type::difference_type;

	using iterator = typename vector_type::iterator;
	using const_iterator = typename vector_type::const_iterator;
	using reverse_iterator = typename vector_type::reverse_iterator;
	using const_reverse_iterator = typename vector_type::const_reverse_iterator;

	flat_map() = default;
	flat_map( const std::initializer_list<std::pair<Key,T>> &il )
	{
		_flatlist.reserve( il.size() );
		for ( auto &it : il )
			insert( it );
	}
	template<typename IT>
	flat_map( IT beg, IT en )
	{
		_flatlist.reserve( std::distance( beg, en ) );
		while ( beg != en )
		{
			insert( *beg );
			++beg;
		}
	}
	~flat_map() = default;
	inline bool empty() const { return _flatlist.empty(); }
	inline size_type size() const { return _flatlist.size(); }
	inline void clear() { _flatlist.clear(); }
	inline void reserve( size_t s ) { _flatlist.reserve( s ); }
	inline iterator begin() { return _flatlist.begin(); }
	inline iterator end() { return _flatlist.end(); }
	inline const_iterator begin() const { return _flatlist.begin(); }
	inline const_iterator end() const { return _flatlist.end(); }
	inline iterator find( const key_type &k )
	{
		auto it = lower_bound( k );
		return it != end() and key_equal( it->first, k ) ? it : end();
	}
	inline const_iterator find( const key_type &k ) const
	{
		auto it = lower_bound( k );
		return it != end() and key_equal( it->first, k ) ? it : end();
	}
	inline size_type count( const key_type &k ) const
	{
		auto it = lower_bound( k );
		return it == end() ? 0 : 1;
	}
	inline iterator lower_bound( const key_type &k ) { return std::lower_bound( begin(), end(), k, &key_less2 ); }
	inline const_iterator lower_bound( const key_type &k ) const { return std::lower_bound( begin(), end(), k, &key_less2 ); }
	inline iterator upper_bound( const key_type &k ) { return std::upper_bound( begin(), end(), k, &key_less2 ); }
	inline const_iterator upper_bound( const key_type &k ) const { return std::upper_bound( begin(), end(), k, &key_less2 ); }
	mapped_type &operator[]( const key_type &k )
	{
		auto it = lower_bound( k );
		if ( it == end() )
		{
			_flatlist.push_back( std::make_pair( k, T() ) );
			it = _flatlist.end() - 1;
		}
		else if ( not key_equal( it->first, k ) )
			it = _flatlist.insert( it, std::make_pair( k, T() ) );
		return it->second;
	}
	const mapped_type &operator[]( const key_type &k ) const
	{
		auto it = lower_bound( k );
		if ( it == end() )
		{
			_flatlist.push_back( std::make_pair( k, T() ) );
			it = _flatlist.end() - 1;
		}
		else if ( not key_equal( it->first, k ) )
			it = _flatlist.insert( it, std::make_pair( k, T() ) );
		return it->second;
	}

	void insert( const value_type &v )
	{
		auto it = std::lower_bound( begin(), end(), v.first, &key_less2 );
		if ( it == end() )
			_flatlist.push_back( v );
		else if ( not key_equal( it->first, v.first ) )
			_flatlist.insert( it, v );
		else
			*it = v;
	}

	template <typename... Args>
	void emplace( const key_type &k, Args &&... args )
	{
		auto it = std::lower_bound( begin(), end(), k, &key_less2 );
		if ( it == end() )
			_flatlist.emplace_back( k, std::forward<Args>( args )... );
		else if ( not key_equal( it->first, k ) )
			_flatlist.emplace( it, k, std::forward<Args>( args )... );
		else
			it->second = T( std::forward<Args>( args )... );
	}

	inline void erase( iterator it ) { _flatlist.erase( it ); }
	inline void erase( const key_type &k ) { erase( lower_bound( k ) ); }
	inline void swap( flat_map<Key, T, CMP> &i_other ) { _flatlist.swap( i_other._flatlist ); }
	inline const vector_type &storage() const { return _flatlist; }
	inline vector_type &storage() { return _flatlist; }

	bool operator<( const flat_map &rhs ) const
	{
		return _flatlist < rhs._flatlist;
	}
	bool operator==( const flat_map &rhs ) const
	{
		return _flatlist == rhs._flatlist;
	}
  private:
	vector_type _flatlist;

	static inline bool key_less2( const value_type &a, const key_type &b ) { return CMP()( a.first, b ); }
	static inline bool key_equal( const key_type &a, const key_type &b ) { return not CMP()( a, b ) and not CMP()( b, a ); }
};

}

#endif
