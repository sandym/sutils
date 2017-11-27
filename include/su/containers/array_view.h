/*
 *  array_view.h
 *  sutils
 *
 *  Created by Sandy Martel on 22/11/2017.
 *  Copyright (c) 2015年 Sandy Martel. All rights reserved.
 *
 * Permission to use, copy, modify, distribute, and sell this software for any
 * purpose is hereby granted without fee. The sotware is provided "AS-IS" and
 * without warranty of any kind, express, implied or otherwise.
 *
 */

#ifndef H_SU_ARRAY_VIEW
#define H_SU_ARRAY_VIEW

namespace su {

template<typename T>
class array_view
{
public:
    // types:
    typedef T & reference;
    typedef const T & const_reference;
    typedef T * iterator;
    typedef const T * const_iterator;
    typedef std::size_t size_type;
    typedef std::ptrdiff_t difference_type;
    typedef T value_type;
    typedef T* pointer;
    typedef const T* const_pointer;
    typedef std::reverse_iterator<iterator> reverse_iterator;
    typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

    array_view() = default;
	array_view( pointer i_data, size_type i_len )
		: _data( i_data ), _len( i_len ){}

    void swap( array_view & a ) noexcept
	{
		std::swap( _data, a._data );
		std::swap( _len, a._len );
	}

    // iterators:
    iterator begin() noexcept { return _data; }
    const_iterator begin() const noexcept { return _data; }
    iterator end() noexcept { return _data + _len; }
    const_iterator end() const noexcept { return _data + _len; }

    reverse_iterator rbegin() noexcept { return _data + _len - 1; }
    const_reverse_iterator rbegin() const noexcept { return _data + _len - 1; }
    reverse_iterator rend() noexcept { return _data - 1; }
    const_reverse_iterator rend() const noexcept { return _data - 1; }

    const_iterator cbegin() const noexcept { return _data; }
    const_iterator cend() const noexcept { return _data + _len; }
    const_reverse_iterator crbegin() const noexcept { return _data + _len - 1; }
    const_reverse_iterator crend() const noexcept { return _data - 1; }

    // capacity:
    constexpr size_type size() const noexcept { return _len; }
    constexpr bool empty() const noexcept { return _len == 0; }

    // element access:
    reference operator[](size_type n) { return _data[n]; }
    const_reference operator[](size_type n) const { return _data[n]; }
    const_reference at(size_type n) const
	{
		if ( n >= _len )
			throw std::out_of_range( "array_view::at" );
		return _data[n];
	}
    reference at(size_type n)
	{
		if ( n >= _len )
			throw std::out_of_range( "array_view::at" );
		return _data[n];
	}

    reference front() { return *_data; }
    const_reference front() const { return *_data; }
    reference back() { return *(_data+_len-1); }
    const_reference back() const { return *(_data+_len-1); }

    T* data() noexcept { return _data; }
    const T* data() const noexcept { return _data; }

private:
	T *_data = nullptr;
	size_type _len = 0;
};

}

#endif
