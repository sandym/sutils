//
//  su_attachable.cpp
//  sutils
//
//  Created by Sandy Martel on 2013/09/22.
//  Copyright (c) 2015年 Sandy Martel. All rights reserved.
//
// Permission to use, copy, modify, distribute, and sell this software for any purpose is hereby
// granted without fee. The sotware is provided "AS-IS" and without warranty of any kind, express,
// implied or otherwise.

#include "su_attachable.h"
#include <cassert>

namespace su
{
attachable::~attachable()
{
	std::for_each( _attachments.begin(), _attachments.end(), []( su::flat_map<std::string, attachment *>::value_type& v )
		{
			v.second->_attachable = nullptr;
			delete v.second;
		} );
}

void attachable::attach( const std::string& i_name, attachment* i_attachment )
{
	detach( i_name );

	if ( i_attachment == nullptr )
		return;

	assert( i_attachment->_attachable == nullptr );
	i_attachment->_attachable = this;
	_attachments.emplace( i_name, i_attachment );
}

void attachable::detach( const std::string& i_name )
{
	auto it = _attachments.find( i_name );
	if ( it != _attachments.end() )
	{
		it->second->_attachable = nullptr;
		delete it->second;
		_attachments.erase( it );
	}
}

void attachable::detach( attachment* i_attachment )
{
	for ( auto it = _attachments.begin(); it != _attachments.end(); ++it )
	{
		if ( it->second == i_attachment )
		{
			_attachments.erase( it );
			break;
		}
	}
}

#if 0
#pragma mark -
#endif

attachment::~attachment()
{
	if ( _attachable != nullptr )
		_attachable->detach( this );
}
}
