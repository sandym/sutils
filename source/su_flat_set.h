/*
 *  su_flat_set.h
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

#ifndef H_SU_FLAT_SET
#define H_SU_FLAT_SET

#include <vector>
#include <algorithm>
#include <functional>

namespace su
{
/*!
	A set implemented with a vector for better cache coherency
*/
template <class T, class CMP = std::less<T>>
class flat_set
{
  public:
	// types:
	using vector_type = std::vector<T>;
	using value_type = typename vector_type::value_type;
	using reference = typename vector_type::reference;
	using const_reference = typename vector_type::const_reference;
	using pointer = typename vector_type::pointer;
	using const_pointer = typename vector_type::const_pointer;
	using size_type = typename vector_type::size_type;
	using difference_type = typename vector_type::difference_type;

	using const_iterator = typename vector_type::const_iterator;
	using const_reverse_iterator = typename vector_type::const_reverse_iterator;

	flat_set() {}
	~flat_set() {}
	inline bool empty() const { return _flatlist.empty(); }
	inline size_type size() const { return _flatlist.size(); }
	inline void clear() { _flatlist.clear(); }
	inline void reserve( size_t s ) { _flatlist.reserve( s ); }
	inline const_iterator begin() const { return _flatlist.begin(); }
	inline const_iterator end() const { return _flatlist.end(); }
	inline const_iterator find( const value_type &v ) const
	{
		auto it = lower_bound( v );
		return it != end() and value_equal( *it, v ) ? it : end();
	}
	inline const_iterator lower_bound( const value_type &v ) const { return std::lower_bound( begin(), end(), v, &value_less ); }
	inline const_iterator upper_bound( const value_type &v ) const { return std::upper_bound( begin(), end(), v, &value_less ); }
	void insert( const value_type &v )
	{
		auto it = std::lower_bound( _flatlist.begin(), _flatlist.end(), v, &value_less );
		if ( it == end() )
			_flatlist.push_back( v );
		else if ( not value_equal( *it, v ) )
			_flatlist.insert( it, v );
		else
			*it = v;
	}

	inline void erase( const_iterator it ) { _flatlist.erase( it ); }
	inline void swap( flat_set<T, CMP> &i_other ) { _flatlist.swap( i_other._flatlist ); }
	inline const vector_type &storage() const { return _flatlist; }
	inline vector_type &storage() { return _flatlist; }
  private:
	vector_type _flatlist;

	static inline bool value_less( const value_type &a, const value_type &b ) { return CMP()( a, b ); }
	static inline bool value_equal( const value_type &a, const value_type &b ) { return not CMP()( a, b ) and not CMP()( b, a ); }
};
}

#endif