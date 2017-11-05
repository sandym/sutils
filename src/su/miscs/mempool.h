/*
 *  mempool.h
 *  sutils
 *
 *  Created by Sandy Martel on 2015/09/03.
 *  Copyright (c) 2017年 Sandy Martel. All rights reserved.
 *
 * Permission to use, copy, modify, distribute, and sell this software for any
 * purpose is hereby granted without fee. The sotware is provided "AS-IS" and
 * without warranty of any kind, express, implied or otherwise.
 */

#ifndef H_SU_MEMPOOL
#define H_SU_MEMPOOL

#include <type_traits>
#include <cstddef>
#include <utility>

namespace su {

template<int SIZE=4096>
class mempool
{
public:
	mempool() = default;
	mempool( const mempool & ) = delete;
	mempool &operator=( const mempool & ) = delete;
	mempool( mempool &&rhs )
	{
		_head = std::exchange( rhs._head, nullptr );
		_current = std::exchange( rhs._current, SIZE );
	}
	mempool &operator=( mempool &&rhs )
	{
		if ( this != &rhs )
		{
			while ( _head != nullptr )
			{
				auto ptr = _head;
				_head = _head->next;
				delete ptr;
			}
			
			_head = std::exchange( rhs._head, nullptr );
			_current = std::exchange( rhs._current, SIZE );
		}
		return *this;
	}
	
	~mempool()
	{
		while ( _head != nullptr )
		{
			auto ptr = _head;
			_head = _head->next;
			delete ptr;
		}
	}
	
	template<typename T,typename... Args>
	std::enable_if_t<std::is_trivially_destructible<T>::value,T*> alloc( Args... args )
	{
		// can't allocate element larger than SIZE
		static_assert( sizeof(T) <= SIZE, "" );
		
		// align the next allocation with the current request
		_current = (_current + alignof(T)-1) & ~(alignof(T)-1);
		
		if ( (_current + sizeof(T)) > SIZE )
		{
			// need a new block
			auto newBlock = new block;
			newBlock->next = _head;
			_head = newBlock;
			_current = 0;
		}
		auto ptr = new (_head->buffer + _current) T( args... );
		_current += sizeof(T);
		return ptr;
	}
	
private:
	struct block
	{
		char buffer[SIZE];
		block *next = nullptr;
	};
	block *_head = nullptr;
	std::size_t _current = SIZE;
};

}

#endif
