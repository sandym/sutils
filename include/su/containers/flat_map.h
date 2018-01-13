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
template<typename Key,
			typename T,
			class CMP=std::less<Key>,
			class STORAGE=std::vector<std::pair<Key,T>>>
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

	class value_compare
	{
		friend class flat_map;
		protected:
			key_compare comp;

			value_compare( key_compare c ) : comp( c ){}
		public:
			bool operator() ( const value_type &lhs, const value_type &rhs ) const;
	};

	// construct/copy/destroy:
	flat_map() = default;
	explicit flat_map( const key_compare &comp )
		: _pair( {}, comp ){}
	flat_map( const key_compare &comp, const allocator_type &a )
		: _pair( { a }, comp ){}
	template <class InputIterator>
	flat_map(InputIterator first, InputIterator last, const key_compare &comp = key_compare() )
		: _pair( storage_type{ first, last }, comp )
	{
		sort();
	}
	template <class InputIterator>
	flat_map( InputIterator first, InputIterator last, const key_compare &comp, const allocator_type& a)
		: _pair( { first, last, a }, comp )
	{
		sort();
	}
	flat_map( const flat_map &m ) = default;
	flat_map( flat_map &&m ) noexcept(
			std::is_nothrow_move_constructible_v<allocator_type> &&
			std::is_nothrow_move_constructible_v<key_compare> ) = default;
	explicit flat_map( const allocator_type &a )
		: _pair( {a}, {} ){}
	flat_map( const flat_map &m, const allocator_type &a )
		: _pair( { m.storage(), a }, {} ){}
//	flat_map( flat_map &&m, const allocator_type &a )
//		: _pair( { m.storage(), a }, {} ){}
	flat_map( std::initializer_list<value_type> il, const key_compare &comp = key_compare() )
		: _pair( il, comp  )
	{
		sort();
	}
	flat_map( std::initializer_list<value_type> il, const key_compare& comp, const allocator_type &a )
		: _pair( { il, a }, comp  )
	{
		sort();
	}
	template<class InputIterator>
	flat_map( InputIterator first, InputIterator last, const allocator_type &a )
		: _pair( { first, last, a }, {}  )
	{
		sort();
	}
	flat_map( std::initializer_list<value_type> il, const allocator_type &a )
		: _pair( { il, a }, {}  )
	{
		sort();
	}
	~flat_map() = default;

//	flat_map &operator=( const flat_map &m );
//	flat_map &operator=( flat_map &&m ) noexcept(
//				allocator_type::propagate_on_container_move_assignment::value &&
//				std::is_nothrow_move_assignable_v<allocator_type> &&
//				std::is_nothrow_move_assignable_v<key_compare>);
//	flat_map &operator=( std::initializer_list<value_type> il );

	// iterators:
	iterator begin() noexcept { return storage().begin(); }
	const_iterator begin() const noexcept { return storage().begin(); }
	iterator end() noexcept { return storage().end(); }
	const_iterator end() const noexcept { return storage().end(); }

	reverse_iterator rbegin() noexcept { return storage().rbegin(); }
	const_reverse_iterator rbegin() const noexcept { return storage().rbegin(); }
	reverse_iterator rend() noexcept { return storage().rend(); }
	const_reverse_iterator rend() const noexcept { return storage().rend(); }

	const_iterator cbegin() const noexcept { return storage().cbegin(); }
	const_iterator cend() const noexcept { return storage().cend(); }
	const_reverse_iterator crbegin() const noexcept { return storage().crbegin(); }
	const_reverse_iterator crend() const noexcept { return storage().crend(); }

	// capacity:
	bool empty() const noexcept { return storage().empty(); }
	size_type size() const noexcept { return storage().size(); }
	size_type max_size() const noexcept { return storage().max_size(); }
	void reserve( size_t s ) { storage().reserve( s ); }
	
	// element access:
	mapped_type &operator[]( const key_type &k )
	{
		auto it = lower_bound( k );
		if ( it == end() )
		{
			storage().emplace_back( k, mapped_type() );
			it = storage().end() - 1;
		}
		else if ( not key_equal( it->first, k ) )
			it = storage().insert( it, std::make_pair( k, mapped_type() ) );
		return it->second;
	}
	mapped_type &operator[]( key_type &&k )
	{
		auto it = lower_bound( k );
		if ( it == end() )
		{
			storage().emplace_back( std::move(k), mapped_type() );
			it = storage().end() - 1;
		}
		else if ( not key_equal( it->first, k ) )
			it = storage().insert( it, std::make_pair( std::move(k), mapped_type() ) );
		return it->second;
	}

#if 0
	mapped_type &at( const key_type &k );
	const mapped_type &at( const key_type &k ) const;

	// modifiers:
	template<class... Args>
	std::pair<iterator,bool> emplace( const key_type& k, Args&&... args );
	template<class... Args>
	iterator emplace_hint( const_iterator position, Args&&... args );
#endif

	std::pair<iterator, bool> insert( const value_type &v )
	{
		auto it = std::lower_bound( begin(), end(), v.first,
					[this]( auto &a, auto &b )
					{
						return key_comp()( a.first, b );
					} );
		if ( it == end() )
		{
			storage().push_back( v );
			return { storage().end()-1, true };
		}
		else if ( not key_equal( it->first, v.first ) )
			return { storage().insert( it, v ), true };
		else
			return { it, false };
	}
#if 0
	std::pair<iterator,bool> insert( value_type &&v );
	template <class P>
	std::pair<iterator,bool> insert( P &&p );
	iterator insert( const_iterator position, const value_type &v );
	iterator insert( const_iterator position, value_type &&v );
	template<class P>
	iterator insert( const_iterator position, P &&p );
	template <class InputIterator>
	void insert( InputIterator first, InputIterator last );
	void insert( std::initializer_list<value_type> il );
#endif

	template <class... Args>
	std::pair<iterator, bool> try_emplace(const key_type& k, Args&&... args)
	{
		auto it = std::lower_bound( begin(), end(), k,
					[this]( auto &a, auto &b )
					{
						return key_comp()( a.first, b );
					} );
		if ( it == end() )
		{
			storage().emplace_back( k, std::forward<Args>( args )... );
			return { storage().end()-1, true };
		}
		else if ( not key_equal( it->first, k ) )
			return { storage().emplace( it, k, std::forward<Args>( args )... ), true };
		else
			return { it, false };
	}
#if 0
	template <class... Args>
	std::pair<iterator, bool> try_emplace(key_type&& k, Args&&... args);
	template <class... Args>
	iterator try_emplace(const_iterator hint, const key_type& k, Args&&... args);
	template <class... Args>
	iterator try_emplace(const_iterator hint, key_type&& k, Args&&... args);
	template <class M>
	std::pair<iterator, bool> insert_or_assign(const key_type& k, M&& obj);
	template <class M>
	std::pair<iterator, bool> insert_or_assign(key_type&& k, M&& obj);
	template <class M>
	iterator insert_or_assign(const_iterator hint, const key_type& k, M&& obj);
	template <class M>
	iterator insert_or_assign(const_iterator hint, key_type&& k, M&& obj);
#endif

	iterator erase( const_iterator it )
	{
		return storage().erase( it );
	}
	iterator erase( iterator it )
	{
		return storage().erase( it );
	}
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
	iterator erase(const_iterator first, const_iterator last)
	{
		return storage().erase( first, last );
	}
	void clear() noexcept { storage().clear(); }

	void swap( flat_map &m )
		noexcept( std::allocator_traits<allocator_type>::is_always_equal::value &&
		std::is_nothrow_swappable_v<key_compare>)
	{
		std::swap( _pair, m._pair );
	}

	// observers:
	allocator_type get_allocator() const noexcept
	{
		return storage().get_allocator();
	}
	key_compare key_comp() const { return _pair; }
	value_compare value_comp() const;

	// map operations:
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
	template<typename K>
	iterator find( const K &k )
	{
		auto it = lower_bound( k );
		return it != end() and key_equal( it->first, k ) ? it : end();
	}
	template<typename K>
	const_iterator find( const K &k ) const
	{
		auto it = lower_bound( k );
		return it != end() and key_equal( it->first, k ) ? it : end();
	}

	template<typename K>
	size_type count( const K &k ) const
	{
		auto it = lower_bound( k );
		return it == end() and key_equal( it->first, k ) ? 0 : 1;
	}
	size_type count( const key_type &k ) const
	{
		auto it = lower_bound( k );
		return it == end() and key_equal( it->first, k ) ? 0 : 1;
	}

	iterator lower_bound( const key_type &k)
	{
		return std::lower_bound( begin(), end(), k,
					[this]( auto &a, auto &b )
					{
						return key_comp()( a.first, b );
					} );
	}
	const_iterator lower_bound( const key_type &k ) const
	{
		return std::lower_bound( begin(), end(), k,
					[this]( auto &a, auto &b )
					{
						return key_comp()( a.first, b );
					} );
	}
	template<typename K>
	iterator lower_bound( const K &k )
	{
		return std::lower_bound( begin(), end(), k,
					[this]( auto &a, auto &b )
					{
						return key_comp()( a.first, b );
					} );
	}
	template<typename K>
	const_iterator lower_bound( const K &k ) const
	{
		return std::lower_bound( begin(), end(), k,
					[this]( auto &a, auto &b )
					{
						return key_comp()( a.first, b );
					} );
	}

	iterator upper_bound( const key_type &k )
	{
		return std::upper_bound( begin(), end(), k,
					[this]( auto &a, auto &b )
					{
						return key_comp()( a.first, b );
					} );
	}
	const_iterator upper_bound( const key_type &k) const
	{
		return std::upper_bound( begin(), end(), k,
					[this]( auto &a, auto &b )
					{
						return key_comp()( a.first, b );
					} );
	}
	template<typename K>
	iterator upper_bound( const K &k )
	{
		return std::upper_bound( begin(), end(), k,
					[this]( auto &a, auto &b )
					{
						return key_comp()( a.first, b );
					} );
	}
	template<typename K>
	const_iterator upper_bound( const K &k ) const
	{
		return std::upper_bound( begin(), end(), k,
					[this]( auto &a, auto &b )
					{
						return key_comp()( a.first, b );
					} );
	}

#if 0
	std::pair<iterator,iterator> equal_range( const key_type &k );
	std::pair<const_iterator,const_iterator> equal_range( const key_type &k ) const;
	template<typename K>
	std::pair<iterator,iterator> equal_range( const K &k );
	template<typename K>
	std::pair<const_iterator,const_iterator> equal_range( const K &k ) const;
#endif

	const storage_type &storage() const { return _pair.storage; }
	storage_type &storage() { return _pair.storage; }
	void sort()
	{
		std::sort( storage().begin(), storage().end(),
				[this]( auto &a, auto &b )
				{
					return key_comp()( a.first, b.first );
				} );
	}

private:
	struct compress_pair : key_compare
	{
		compress_pair() = default;
		compress_pair( const storage_type &s, const key_compare &k )
			: key_compare( k ), storage( s ){}
		storage_type storage;
	} _pair;

	template<typename K>
	inline bool key_equal( const key_type &a, const K &b ) const
	{
		return not key_comp()( a, b ) and not key_comp()( b, a );
	}
};

}

template<class Key, class T, class CMP, class STORAGE>
inline bool operator==( const su::flat_map<Key,T,CMP,STORAGE> &lhs,
			const su::flat_map<Key,T,CMP,STORAGE> &rhs )
{
	return lhs.storage() == rhs.storage();
}
template<class Key, class T, class CMP, class STORAGE>
inline bool operator<( const su::flat_map<Key,T,CMP,STORAGE> &lhs,
			const su::flat_map<Key,T,CMP,STORAGE> &rhs)
{
	return lhs.storage() < rhs.storage();
}
template<class Key, class T, class CMP, class STORAGE>
inline bool operator!=(const su::flat_map<Key,T,CMP,STORAGE> &lhs,
			const su::flat_map<Key,T,CMP,STORAGE> &rhs)
{
	return lhs.storage() != rhs.storage();
}
template<class Key, class T, class CMP, class STORAGE>
inline bool operator>(const su::flat_map<Key,T,CMP,STORAGE> &lhs,
			const su::flat_map<Key,T,CMP,STORAGE> &rhs)
{
	return lhs.storage() > rhs.storage();
}
template<class Key, class T, class CMP, class STORAGE>
inline bool operator>=(const su::flat_map<Key,T,CMP,STORAGE> &lhs,
			const su::flat_map<Key,T,CMP,STORAGE> &rhs)
{
	return lhs.storage() >= rhs.storage();
}
template<class Key, class T, class CMP, class STORAGE>
inline bool operator<=(const su::flat_map<Key,T,CMP,STORAGE> &lhs,
			const su::flat_map<Key,T,CMP,STORAGE> &rhs)
{
	return lhs.storage() <= rhs.storage();
}

#endif
