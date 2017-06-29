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

#include <ciso646>

namespace su {

template<class T,int SMALL_SIZE=4>
class small_vector
{
public:
	typedef T value_type;
	typedef T &reference;
	typedef const T &const_reference;
	typedef T *iterator;
	typedef const T *const_iterator;
	typedef std::size_t size_type;
	typedef std::ptrdiff difference_type;
	typedef T *pointer;
	typedef const T *const_pointer;
	typedef std::reverse_iterator<iterator> reverse_iterator;
	typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

//	small_vector();
//	explicit small_vector(size_type n);
//	small_vector(size_type n, const value_type& value );
//	template <class InputIterator>
//		small_vector(InputIterator first, InputIterator last );
//	small_vector(const small_vector& x);
//	small_vector( small_vector&& x );
//	small_vector(initializer_list<value_type> il);
	~small_vector()
	{
		if ( _size > SMALL_SIZE )
			delete [] u._vec.data;
	}
//	small_vector& operator=(const small_vector& x);
//	small_vector& operator=(small_vector&& x) noexcept;
//	small_vector& operator=(initializer_list<value_type> il);
//	template <class InputIterator>
//		void assign(InputIterator first, InputIterator last);
//	void assign(size_type n, const value_type& u);
//	void assign(initializer_list<value_type> il);

	iterator begin() noexcept { return _size > SMALL_SIZE ? u._vec.data : u._small_vec.data; }
	const_iterator begin() const noexcept { return _size > SMALL_SIZE ? u._vec.data : u._small_vec.data; }
	iterator end() noexcept { return (_size > SMALL_SIZE ? u._vec.data : u._small_vec.data) + _size; }
	const_iterator end() const noexcept { return (_size > SMALL_SIZE ? u._vec.data : u._small_vec.data) + _size; }
//
//	reverse_iterator	   rbegin() noexcept;
//	const_reverse_iterator rbegin()  const noexcept;
//	reverse_iterator	   rend() noexcept;
//	const_reverse_iterator rend()	const noexcept;

	const_iterator cbegin() const noexcept { return _size > SMALL_SIZE ? u._vec.data : u._small_vec.data; }
	const_iterator cend() const noexcept { return (_size > SMALL_SIZE ? u._vec.data : u._small_vec.data) + _size; }
//	const_reverse_iterator crbegin() const noexcept;
//	const_reverse_iterator crend()   const noexcept;
//
	size_type size() const noexcept { return _size; }
//	size_type max_size() const noexcept;
//	size_type capacity() const noexcept;
	bool empty() const noexcept { return _size == 0; }
//	void reserve(size_type n);
//	void shrink_to_fit() noexcept;

	reference operator[](size_type n) { return _size > SMALL_SIZE ? u._vec.data[n] : u._small_vec.data[n]; }
	const_reference operator[](size_type n) const { return _size > SMALL_SIZE ? u._vec.data[n] : u._small_vec.data[n]; }
//	reference	   at(size_type n);
//	const_reference at(size_type n) const;
//
	reference front() { return _size > SMALL_SIZE ? u._vec.data[0] : u._small_vec.data[0]; }
	const_reference front() const { return _size > SMALL_SIZE ? u._vec.data[0] : u._small_vec.data[0]; }
	reference back() { return _size > SMALL_SIZE ? u._vec.data[_size-1] : u._small_vec.data[_size-1]; }
	const_reference back() const { return _size > SMALL_SIZE ? u._vec.data[_size-1] : u._small_vec.data[_size-1]; }

	value_type *data() noexcept { return _size > SMALL_SIZE ? u._vec.data : u._small_vec.data; }
	const value_type *data() const noexcept { return _size > SMALL_SIZE ? u._vec.data : u._small_vec.data; }

//	void push_back(const value_type& x);
//	void push_back(value_type&& x);
//	template <class... Args>
//		void emplace_back(Args&&... args);
//	void pop_back();
//
//	template <class... Args> iterator emplace(const_iterator position, Args&&... args);
//	iterator insert(const_iterator position, const value_type& x);
//	iterator insert(const_iterator position, value_type&& x);
//	iterator insert(const_iterator position, size_type n, const value_type& x);
//	template <class InputIterator>
//		iterator insert(const_iterator position, InputIterator first, InputIterator last);
//	iterator insert(const_iterator position, initializer_list<value_type> il);
//
//	iterator erase(const_iterator position);
//	iterator erase(const_iterator first, const_iterator last);
//
//	void clear() noexcept;
//
//	void resize(size_type sz);
//	void resize(size_type sz, const value_type& c);
//
//	void swap(small_vector&) noexcept;

private:
	size_type _size = 0;
	struct vector
	{
		T *data = nullptr;
		size_type cap = 0;
	};
	struct small_vector
	{
		T data[SMALL_SIZE];
	};
	union U
	{
		vector _vec;
		small_vector _small_vec;
	} u;
	
};

}

#endif
