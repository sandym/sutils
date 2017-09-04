/*
 *  su_small_vector.h
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

#ifndef H_SU_SMALL_VECTOR
#define H_SU_SMALL_VECTOR

#include <cstddef>
#include <memory>
#include <algorithm>
#include <stdexcept>
#include <iterator>

namespace su {

template<class T,std::size_t N,class Allocator=std::allocator<T>>
class small_vector
{
public:
	typedef T value_type;
	typedef Allocator allocator_type;
	typedef typename allocator_type::reference reference;
	typedef typename allocator_type::const_reference const_reference;
	typedef T *iterator;
	typedef const T *const_iterator;
	typedef typename allocator_type::size_type size_type;
	typedef typename allocator_type::difference_type difference_type;
	typedef typename allocator_type::pointer pointer;
	typedef typename allocator_type::const_pointer const_pointer;
	typedef std::reverse_iterator<iterator> reverse_iterator;
	typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

	small_vector() noexcept(std::is_nothrow_default_constructible_v<allocator_type>) = default;
	explicit small_vector( const allocator_type &i_alloc ) : _compp( i_alloc ){}
//explicit small_vector( size_type n );
//explicit small_vector( size_type n, const allocator_type &i_alloc );
//small_vector( size_type n, const value_type &value, const allocator_type &i_alloc = {} );
//template <class InputIterator>
//small_vector( InputIterator first, InputIterator last, const allocator_type &i_alloc = {} );
//small_vector( const small_vector &x );
//small_vector( small_vector &&x ) noexcept(std::is_nothrow_move_constructible_v<allocator_type>);
//small_vector( std::initializer_list<value_type> il );
//small_vector( std::initializer_list<value_type> il, const allocator_type &i_alloc );
	~small_vector()
	{
		//if constexpr ( not std::is_pod_v<T> )
		{
			for ( auto ptr = data(); ptr != _end; ++ptr )
				get_alloc().destroy( ptr );
		}
		if ( not is_small() )
			get_alloc().deallocate( u._vec._storage, capacity() );
	}
//small_vector &operator=( const small_vector &x );
//small_vector &operator=( small_vector &&x ) noexcept(allocator_type::propagate_on_container_move_assignment::value and std::is_nothrow_move_assignable_v<allocator_type>);
//small_vector &operator=( std::initializer_list<value_type> il );
//template <class InputIterator>
//void assign( InputIterator first, InputIterator last );
//void assign( size_type n, const value_type &u );
//void assign( std::initializer_list<value_type> il );

	allocator_type get_allocator() const noexcept { return get_alloc(); }

	iterator begin() noexcept { return data(); }
	const_iterator begin() const noexcept { return data(); }
	iterator end() noexcept { return data() + size(); }
	const_iterator end() const noexcept { return data() + size(); }

	reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }
	const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(end()); }
	reverse_iterator rend() noexcept { return reverse_iterator(begin()); }
	const_reverse_iterator rend() const noexcept { return const_reverse_iterator(begin()); }

	const_iterator cbegin() const noexcept { return begin(); }
	const_iterator cend() const noexcept { return end(); }
	const_reverse_iterator crbegin() const noexcept { return rbegin(); }
	const_reverse_iterator crend() const noexcept { return rend(); }

	size_type size() const noexcept { return _end - data(); }
	size_type max_size() const noexcept { return std::min<size_type>( get_alloc().max_size(), std::numeric_limits<size_type>::max() / 2 ); }
	size_type capacity() const noexcept { return _compp._cap; }
	bool empty() const noexcept { return _end == data(); }
	void reserve( size_type n )
	{
		if ( n > capacity() )
			grow( n );
	}
//void shrink_to_fit() noexcept;

	reference operator[](size_type n) { return data()[n]; }
	const_reference operator[](size_type n) const { return data()[n]; }
	reference at(size_type n) { if (n >= size() ) throw std::out_of_range("small_vector"); return data()[n]; }
	const_reference at(size_type n) const { if (n >= size() ) throw std::out_of_range("small_vector"); return data()[n]; }

	reference front() { return *data(); }
	const_reference front() const { return *data(); }
	reference back() { return *(data()+size()-1); }
	const_reference back() const { return *(data()+size()-1); }

	value_type *data() noexcept { return is_small() ? u._small_vec.data() : u._vec.data(); }
	const value_type *data() const noexcept { return is_small() ? u._small_vec.data() : u._vec.data(); }

	void push_back( const value_type &x )
	{
		if ( (size() + 1) > capacity() )
			grow( is_small() ? N * 2 : size() * 2 );

		get_alloc().construct( _end, x );
		++_end;
	}
	void push_back( value_type &&x )
	{
		if ( (size() + 1) > capacity() )
			grow( is_small() ? N * 2 : size() * 2 );

		get_alloc().construct( _end, std::move(x) );
		++_end;
	}
	template <class... Args>
	void emplace_back( Args&&... args )
	{
		if ( (size() + 1) > capacity() )
			grow( is_small() ? N * 2 : size() * 2 );

		get_alloc().construct( _end, std::forward<Args>(args)... );
		++_end;
	}
	void pop_back()
	{
		//if constexpr ( not std::is_pod_v<T> )
			get_alloc().destroy( _end-1 );
		--_end;
	}

	template <class... Args> iterator emplace(const_iterator pos, Args&&... args)
	{
		if ( (size() + 1) > capacity() )
			grow( is_small() ? N * 2 : size() * 2 );
		
		...
		std::move_backward( pos, _end-1, _end );
		get_alloc().construct( pos, std::forward<Args>(args)... );
		++_end;
	}
//	iterator insert( const_iterator position, const value_type &x );
//	iterator insert( const_iterator position, value_type &&x );
//	iterator insert( const_iterator position, size_type n, const value_type &x );
//	template <class InputIterator>
//	iterator insert( const_iterator position, InputIterator first, InputIterator last );
//	iterator insert( const_iterator position, std::initializer_list<value_type> il );

	iterator erase( const_iterator pos )
	{
		std::move( pos+1, _end, pos );
		--_end;
		//if constexpr ( not std::is_pod_v<T> )
			get_alloc().destroy( _end );
		return pos;
	}
	iterator erase( const_iterator first, const_iterator last )
	{
		std::move( last, _end, first );
		auto r = last - first;
		//if constexpr ( not std::is_pod_v<T> )
		{
			for ( auto ptr = _end - r; ptr != _end; ++ptr )
				get_alloc().destroy( ptr );
		}
		_end -= r;
		return first;
	}

	void clear() noexcept
	{
		//if constexpr ( not std::is_pod_v<T> )
		{
			for ( auto ptr = data(); ptr != _end; ++ptr )
				get_alloc().destroy( ptr );
		}
		_end = data();
	}

//void resize( size_type sz );
//void resize( size_type sz, const value_type &c );

//void swap( small_vector &x ) noexcept(std::allocator_traits<allocator_type>::propagate_on_container_swap::value or std::allocator_traits<allocator_type>::is_always_equal::value);

private:
	struct vec
	{
		pointer _storage = nullptr;
		
		pointer data() { return _storage; }
		const_pointer data() const { return _storage; }
	};
	struct small_vec
	{
		char _storage[sizeof(T) * N];

		pointer data() { return reinterpret_cast<pointer>(_storage); }
		const_pointer data() const { return reinterpret_cast<const_pointer>(_storage); }
	};
	union U
	{
		U() : _small_vec() {}
		~U(){}
		small_vec _small_vec;
		vec _vec;
	} u{};
	pointer _end = u._small_vec.data();
	
	// compressed pair for the allocator and capacity
	struct compressed_pair : allocator_type
	{
		size_type _cap = N;
	} _compp;
	const allocator_type &get_alloc() const noexcept { return _compp; }
	allocator_type &get_alloc() noexcept { return _compp; }
	
	bool is_small() const { return capacity() == N; }
	
	void grow( size_type n )
	{
		auto s = size();
		
		auto ptr = get_alloc().allocate( n );
		
		// move from small storage to ptr
		for ( auto dst = ptr, src = data(); src != _end; ++src, ++dst )
			get_alloc().construct( dst, std::move(*src) );
		
		//if constexpr ( not std::is_pod_v<T> )
		{
			// destroy all object in small storage
			for ( auto src = data(); src != _end; ++src )
				get_alloc().destroy( src );
		}
		
		// init u._vec
		u._vec._storage = ptr;
		
		// update _end & capacity
		_end = ptr + s;
		_compp._cap = n;
	}
};

}

//template<class T,class Allocator> bool operator==(const vector<T,Allocator>& x, const vector<T,Allocator>& y);
//template<class T,class Allocator> bool operator< (const vector<T,Allocator>& x, const vector<T,Allocator>& y);
//template<class T,class Allocator> bool operator!=(const vector<T,Allocator>& x, const vector<T,Allocator>& y);
//template<class T,class Allocator> bool operator> (const vector<T,Allocator>& x, const vector<T,Allocator>& y);
//template<class T,class Allocator> bool operator>=(const vector<T,Allocator>& x, const vector<T,Allocator>& y);
//template<class T,class Allocator> bool operator<=(const vector<T,Allocator>& x, const vector<T,Allocator>& y);

#endif
