/*
 *  signal.h
 *  sutils
 *
 *  Created by Sandy Martel on 2015/09/03.
 *  Copyright (c) 2015年 Sandy Martel. All rights reserved.
 *
 * Permission to use, copy, modify, distribute, and sell this software for any
 * purpose is hereby granted without fee. The sotware is provided "AS-IS" and
 * without warranty of any kind, express, implied or otherwise.
 */

#ifndef H_SU_SIGNAL
#define H_SU_SIGNAL

#include "su/miscs/statesaver.h"
#include <ciso646>
#include <functional>
#include <cassert>

namespace su {

/*!
 *  signal class
 *    for signal with no returned value
 *
 *    usage:
 *      // 1- create a signal
 *      su::signal<int> signalTakingAnInt;
 *
 *      // 2- connect something to it
 *      signalTakingAnInt.connect( []( int i ){ std::cout << i << std::endl; } );
 *
 *      // 3- call the signal
 *      signalTakingAnInt( 45 ); // this will print '45'
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
		conn_node( const func_type &i_func, std::unique_ptr<conn_node> &&i_next )
			: func( i_func ), next( std::move(i_next) ){}
		func_type func;
		std::unique_ptr<conn_node> next;
	};
	using conn = conn_node *;

	signal() = default;
	~signal() = default;
	signal( const signal & ) = delete;
	signal &operator=( const signal & ) = delete;

	conn connect( const func_type &i_func )
	{
		if ( not i_func )
			return nullptr;
		
		assert( not _in_signal );
		_head = std::make_unique<conn_node>( i_func, std::move(_head) );
		return _head.get();
	}

	//! invoke the signal
	void operator()( Args... args )
	{
		assert( not _in_signal );
		statesaver<bool> save( _in_signal, false );
		auto ptr = _head.get();
		while ( ptr != nullptr )
		{
			ptr->func( std::forward<Args>( args )... );
			ptr = ptr->next.get();
		}
	}

	void disconnect( conn c )
	{
		if ( c == nullptr )
			return;
		
		assert( not _in_signal );
		if ( c == _head.get() )
		{
			_head = std::move(_head->next);
		}
		else
		{
			auto p = _head.get();
			while ( p != nullptr and p->next.get() != c )
				p = p->next.get();
			if ( p != nullptr )
				p->next = std::move(c->next);
		}
	}

	void disconnectAll()
	{
		assert( not _in_signal );
		_head.reset();
	}

	class scoped_conn
	{
	public:
		scoped_conn( signal &sig, const func_type &i_func )
			: s( sig ) { c = s.connect( i_func ); }
		~scoped_conn() { disconnect(); }
		void disconnect()
		{
			s.disconnect( c );
			c = nullptr;
		}

	private:
		signal &s;
		conn c;
	};

private:
	bool _in_signal = false;
	std::unique_ptr<conn_node> _head;
};
}

#endif
