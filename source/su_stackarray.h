/*
 *  su_stackarray.h
 *  sutils
 *
 *  Created by Sandy Martel on 07-02-25.
 *  Copyright (c) 2015年 Sandy Martel. All rights reserved.
 *
 * Permission to use, copy, modify, distribute, and sell this software for any purpose is hereby
 * granted without fee. The sotware is provided "AS-IS" and without warranty of any kind, express,
 * implied or otherwise.
 */

#ifndef H_SU_STACKARRAY
#define H_SU_STACKARRAY

#include <cstddef>
#include <memory>

namespace su {

/*!
   @brief Automatic heap array.
*/
template <typename T, size_t S = 512>
class stackarray final
{
public:
	stackarray( size_t i_len = S ) : _size( i_len )
	{
		if ( size() > S )
		{
			_heapArray = std::make_unique<T[]>( size() );
			_array = _heapArray.get();
		}
		else
			_array = _stackArray;
	}
	stackarray( const stackarray<T, S> &i_other ) : _size( i_other.size() )
	{
		if ( i_other._array == i_other._stackArray )
			_array = _stackArray;
		else
		{
			_heapArray = std::make_unique<T[]>( size() );
			_array = _heapArray.get();
		}
		std::copy( i_other.data(), i_other.data() + size(), data() );
	}
	stackarray( stackarray<T, S> &&i_other ) noexcept : _size( i_other.size() )
	{
		if ( i_other._array == i_other._stackArray )
		{
			_array = _stackArray;
			std::copy( i_other.data(), i_other.data() + size(), data() );
		}
		else
		{
			_heapArray = std::move(i_other._heapArray);
			_array = _heapArray.get();

			i_other._array = i_other._stackArray;
			i_other._size = S;
		}
	}
	~stackarray() = default;

	stackarray &operator=( const stackarray<T, S> &i_other )
	{
		if ( this != &i_other )
		{
			realloc( i_other.size() );
			std::copy( i_other.data(), i_other.data() + i_other.size(), data() );
		}
		return *this;
	}
	stackarray &operator=( stackarray<T, S> &&i_other ) noexcept
	{
		_size = i_other.size();
		if ( i_other._array == i_other._stackArray )
			std::copy( i_other.data(), i_other.data() + size(), data() );
		else
		{
			_heapArray = std::move(i_other._heapArray);
			_array = _heapArray.get();

			i_other._array = i_other._stackArray;
			i_other._size = S;
		}
		return *this;
	}

	void realloc( size_t i_len )
	{
		if ( i_len > _size ) // never go smaller, this is a temporary array anyway
		{
			_size = i_len;
			_heapArray.reset();

			if ( _size > S )
			{
				_heapArray = std::make_unique<T[]>( _size );
				_array = _heapArray.get();
			}
			else
				_array = _stackArray;
		}
	}

	inline size_t size() const { return _size; }
	inline const T &operator[]( size_t i ) const { return _array[i]; }
	inline T &operator[]( size_t i ) { return _array[i]; }
	inline const T *data() const { return _array; }
	inline T *data() { return _array; }
	inline operator T *() { return _array; }
	inline operator const T *() const { return _array; }
private:
	T _stackArray[S];
	std::unique_ptr<T[]> _heapArray;
	size_t _size = 0;
	T *_array;
};

}

#endif
