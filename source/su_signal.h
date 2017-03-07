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
#include <vector>
#include <cassert>
#include <algorithm>
#include "su_shim/memory.h"

namespace su
{
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

	using conn = func_type *; //!< represent a connection

	signal() = default;
	~signal() = default;
	signal( const signal & ) = delete;
	signal &operator=( const signal & ) = delete;

	conn connect( const func_type &i_func )
	{
		assert( not _in_signal );
		_connections.push_back( su::make_unique<func_type>( i_func ) );
		return _connections.back().get();
	}

	//! invoke the signal
	void operator()( Args... args )
	{
		assert( not _in_signal );
		statesaver<bool> save( _in_signal, false );
		for ( auto &it : _connections )
			( *it )( std::forward<Args>( args )... );
	}

	void disconnect( conn c )
	{
		auto it = std::find_if( std::begin( _connections ), std::end( _connections ),
								[c]( const std::unique_ptr<func_type> &p ) { return p.get() == c; } );
		if ( it != _connections.end() )
		{
			assert( not _in_signal );
			_connections.erase( it );
		}
	}

	void disconnectAll()
	{
		assert( not _in_signal );
		_connections.clear();
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
	std::vector<std::unique_ptr<func_type>> _connections;
};
}

#endif
