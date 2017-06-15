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

namespace su {

attachable::~attachable()
{
}

void attachable::attach( const std::string& i_name, std::unique_ptr<attachment> &&i_attachment )
{
	assert( i_attachment->_attachable == nullptr ); // already attached !?

	detach( i_name );

	if ( i_attachment.get() == nullptr )
		return;

	i_attachment->_attachable = this;
	_attachments.emplace( i_name, std::move(i_attachment) );
}

void attachable::detach( const std::string& i_name )
{
	auto it = _attachments.find( i_name );
	if ( it != _attachments.end() )
		_attachments.erase( it );
}

}
