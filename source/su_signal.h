/*
 *  su_signal.h
 *  sutils
 *
 *  Created by Sandy Martel on 2015/09/03.
 *  Copyright (c) 2015年 Sandy Martel. All rights reserved.
 *
 * Permission to use, copy, modify, distribute, and sell this software for any purpose is hereby
 * granted without fee. The sotware is provided "AS-IS" and without warranty of any kind, express,
 * implied or otherwise.
 */

#ifndef H_SU_SIGNAL
#define H_SU_SIGNAL

#include "su_statesaver.h"
#include <ciso646>
#include <functional>
#include <cassert>

namespace su {

/*!
 *  signal class
 *      for signal with no returned value
 *
 *      usage:
 *          // 1- create a signal
 *          su::signal<int> signalTakingAnInt;
 *
 *          // 2- connect something to it
 *          signalTakingAnInt.connect( []( int i ){ std::cout << i << std::endl; } );
 *
 *          // 3- call the signal
 *          signalTakingAnInt( 45 ); // this will print '45'
 */
template <typename... Args>
class signal final
{
public:
	//! function type for this signal.
	using func_type = std::function<void( Args... )>;

	//! represent a connection
	struct conn_node
	{
		func_type func;
		conn_node *prev = nullptr;
		conn_node *next = nullptr;
	};
	using conn = conn_node *;

	signal() = default;
	~signal()
	{
		disconnectAll();
	}
	signal( const signal & ) = delete;
	signal &operator=( const signal & ) = delete;

	conn connect( const func_type &i_func )
	{
		if ( not i_func )
			return nullptr;
		
		assert( not _in_signal );
		auto c = new conn_node{ i_func, nullptr, _head  };
		if ( _head )
			_head->prev = c;
		_head = c;
		return c;
		
	}

	//! invoke the signal
	void operator()( Args... args )
	{
		assert( not _in_signal );
		statesaver<bool> save( _in_signal, false );
		auto ptr = _head;
		while ( ptr != nullptr )
		{
			ptr->func( std::forward<Args>( args )... );
			ptr = ptr->next;
		}
	}

	void disconnect( conn c )
	{
		if ( c == nullptr )
			return;
		
		assert( not _in_signal );
		if ( c == _head )
		{
			_head = _head->next;
			if ( _head )
				_head->prev = nullptr;
		}
		else
		{
			if ( c->next )
				c->next->prev = c->prev;
			c->prev->next = c->next;
		}
		delete c;
	}

	void disconnectAll()
	{
		assert( not _in_signal );
		while ( _head != nullptr )
		{
			auto c = _head;
			_head = _head->next;
			delete c;
		}
	}

	class scoped_conn
	{
	public:
		scoped_conn( signal &sig, const func_type &i_func ) : s( sig ) { c = s.connect( i_func ); }
		~scoped_conn() { disconnect(); }
		void disconnect()
		{
			if ( c != nullptr )
			{
				s.disconnect( c );
				c = nullptr;
			}
		}

	private:
		signal &s;
		conn c;
	};

private:
	bool _in_signal = false;
	conn_node *_head = nullptr;
};
}

#endif
