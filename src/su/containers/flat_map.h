/*
 *  flat_map.h
 *  sutils
 *
 *  Created by Sandy Martel on 22/11/2011.
 *  Copyright (c) 2015年 Sandy Martel. All rights reserved.
 *
 * Permission to use, copy, modify, distribute, and sell this software for any
 * purpose is hereby granted without fee. The sotware is provided "AS-IS" and
 * without warranty of any kind, express, implied or otherwise.
 *
 */

#ifndef H_SU_FLAT_MAP
#define H_SU_FLAT_MAP

#include <ciso646>
#include <vector>
#include <algorithm>
#include <functional>

namespace su {

/*!
	A map implemented with a vector for better cache coherency
*/
template<typename Key,typename T,class CMP=std::less<Key>,class STORAGE=std::vector<std::pair<Key,T>>>
class flat_map
{
public:
	// types:
	using storage_type = STORAGE;
	using key_type = Key;
	using mapped_type = T;
	using value_type = typename storage_type::value_type;
	using key_compare = CMP;
	using allocator_type = typename storage_type::allocator_type;
	using reference = typename storage_type::reference;
	using const_reference = typename storage_type::const_reference;
	using pointer = typename storage_type::pointer;
	using const_pointer = typename storage_type::const_pointer;
	using size_type = typename storage_type::size_type;
	using difference_type = typename storage_type::difference_type;

	using iterator = typename storage_type::iterator;
	using const_iterator = typename storage_type::const_iterator;
	using reverse_iterator = typename storage_type::reverse_iterator;
	using const_reverse_iterator = typename storage_type::const_reverse_iterator;

	flat_map() = default;
	flat_map( const std::initializer_list<std::pair<key_type,T>> &il )
		: _flatlist( il )
	{
		sort();
	}
	template<typename IT>
	flat_map( IT beg, IT en )
		: _flatlist( beg, en )
	{
		sort();
	}
	~flat_map() = default;
	bool empty() const noexcept { return _flatlist.empty(); }
	size_type size() const noexcept { return _flatlist.size(); }
	size_type max_size() const noexcept { return _flatlist.max_size(); }
	void clear() noexcept { _flatlist.clear(); }
	void reserve( size_t s ) { _flatlist.reserve( s ); }

	iterator begin() noexcept { return _flatlist.begin(); }
	const_iterator begin() const noexcept { return _flatlist.begin(); }
	iterator end() noexcept { return _flatlist.end(); }
	const_iterator end() const noexcept { return _flatlist.end(); }

	reverse_iterator rbegin() noexcept { return _flatlist.rbegin(); }
	const_reverse_iterator rbegin() const noexcept { return _flatlist.rbegin(); }
	reverse_iterator rend() noexcept { return _flatlist.rend(); }
	const_reverse_iterator rend() const noexcept { return _flatlist.rend(); }
	
	const_iterator cbegin() const noexcept { return _flatlist.cbegin(); }
	const_iterator cend() const noexcept { return _flatlist.cend(); }
	const_reverse_iterator crbegin() const noexcept { return _flatlist.crbegin(); }
	const_reverse_iterator crend() const noexcept { return _flatlist.crend(); }

	iterator find( const key_type &k )
	{
		auto it = lower_bound( k );
		return it != end() and key_equal( it->first, k ) ? it : end();
	}
	const_iterator find( const key_type &k ) const
	{
		auto it = lower_bound( k );
		return it != end() and key_equal( it->first, k ) ? it : end();
	}
	size_type count( const key_type &k ) const
	{
		auto it = lower_bound( k );
		return it == end() and key_equal( it->first, k ) ? 0 : 1;
	}
	iterator lower_bound( const key_type &k ) { return std::lower_bound( begin(), end(), k, &key_less2 ); }
	const_iterator lower_bound( const key_type &k ) const { return std::lower_bound( begin(), end(), k, &key_less2 ); }
	iterator upper_bound( const key_type &k ) { return std::upper_bound( begin(), end(), k, &key_less2 ); }
	const_iterator upper_bound( const key_type &k ) const { return std::upper_bound( begin(), end(), k, &key_less2 ); }
	mapped_type &operator[]( const key_type &k )
	{
		auto it = lower_bound( k );
		if ( it == end() )
		{
			_flatlist.emplace_back( k, mapped_type() );
			it = _flatlist.end() - 1;
		}
		else if ( not key_equal( it->first, k ) )
			it = _flatlist.insert( it, std::make_pair( k, mapped_type() ) );
		return it->second;
	}
	mapped_type &operator[]( key_type &&k )
	{
		auto it = lower_bound( k );
		if ( it == end() )
		{
			_flatlist.emplace_back( std::move(k), mapped_type() );
			it = _flatlist.end() - 1;
		}
		else if ( not key_equal( it->first, k ) )
			it = _flatlist.insert( it, std::make_pair( std::move(k), mapped_type() ) );
		return it->second;
	}

	std::pair<iterator,bool> insert( const value_type &v )
	{
		auto it = std::lower_bound( begin(), end(), v.first, &key_less2 );
		if ( it == end() )
		{
			_flatlist.push_back( v );
			return { _flatlist.end()-1, true };
		}
		else if ( not key_equal( it->first, v.first ) )
			return { _flatlist.insert( it, v ), true };
		else
			return { it, false };
	}

	template <typename... Args>
	std::pair<iterator,bool> emplace( const key_type &k, Args &&... args )
	{
		auto it = std::lower_bound( begin(), end(), k, &key_less2 );
		if ( it == end() )
		{
			_flatlist.emplace_back( k, std::forward<Args>( args )... );
			return { _flatlist.end()-1, true };
		}
		else if ( not key_equal( it->first, k ) )
			return { _flatlist.emplace( it, k, std::forward<Args>( args )... ), true };
		else
			return { it, false };
	}

	iterator erase( iterator it ) { return _flatlist.erase( it ); }
	size_type erase( const key_type &k )
	{
		auto it = lower_bound( k );
		if ( it != end() and key_equal( it->first, k ) )
		{
			erase( it );
			return 1;
		}
		else
			return 0;
	}
	void swap( flat_map<key_type,T,key_compare> &i_other ) { _flatlist.swap( i_other._flatlist ); }

	bool operator<( const flat_map &rhs ) const
	{
		return _flatlist < rhs._flatlist;
	}
	bool operator==( const flat_map &rhs ) const
	{
		return _flatlist == rhs._flatlist;
	}

	const storage_type &storage() const { return _flatlist; }
	storage_type &storage() { return _flatlist; }
	void sort()
	{
		std::sort( _flatlist.begin(), _flatlist.end(), []( auto &a, auto &b ){ return key_compare()( a.first, b.first ); } );
	}

private:
	storage_type _flatlist;

	static inline bool key_less2( const value_type &a, const key_type &b ) { return key_compare()( a.first, b ); }
	static inline bool key_equal( const key_type &a, const key_type &b ) { return not key_compare()( a, b ) and not key_compare()( b, a ); }
};

}

#endif
