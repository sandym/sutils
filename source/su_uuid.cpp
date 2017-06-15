/*
 *  su_uuid.cpp
 *  sutils
 *
 *  Created by Sandy Martel on 05-05-07.
 *  Copyright (c) 2015年 Sandy Martel. All rights reserved.
 *
 * Permission to use, copy, modify, distribute, and sell this software for any purpose is hereby
 * granted without fee. The sotware is provided "AS-IS" and without warranty of any kind, express,
 * implied or otherwise.
 */

#include "su_uuid.h"
#include "su_platform.h"
#include "cfauto.h"
#include <ciso646>
#include <cctype>
#if UPLATFORM_LINUX
#include <uuid/uuid.h>
#endif
#if UPLATFORM_WIN
#include <rpc.h>
#endif

namespace su {

/*!
   @brief Create a unique value.

	   High probability to be unique in the universe.
*/
uuid uuid::create()
{
	uuid v;
#if UPLATFORM_MAC || UPLATFORM_IOS
	cfauto<CFUUIDRef> ref( CFUUIDCreate( 0 ) );
	CFUUIDBytes bytes = CFUUIDGetUUIDBytes( ref );
#elif UPLATFORM_LINUX
	uuid_t bytes;
	uuid_generate( bytes );
#else
	::UUID bytes;
	UuidCreate(&bytes);
#endif
	uint8_t *ptr = reinterpret_cast<uint8_t *>( &bytes );
	std::copy( ptr, ptr + 16, v._uuid.data() );

	return v;
}

uuid::uuid( const std::array<uint8_t, 16> &i_value ) : _uuid( i_value )
{
}

uuid::uuid( const std::string &i_value )
{
	int i = 0, j = 0;
	int v[2];
	for ( auto it : i_value )
	{
		if ( std::isxdigit( it ) )
		{
			auto c = std::tolower( it );
			v[i % 2] = c >= 'a' ? c - 'a' + 10 : c - '0';
			if ( ( i % 2 ) == 1 )
			{
				_uuid[j++] = (uint8_t)(( v[0] << 4 ) | v[1]);
				if ( j == 16 )
					return;
			}
			++i;
		}
	}
	// check partial byte
	if ( ( i % 2 ) == 1 )
		_uuid[j++] = (uint8_t)(( v[0] << 4 ));

	// complete with zeros
	while ( j < 16 )
		_uuid[j++] = 0;
}

std::string uuid::string() const
{
	std::string uuid;
	uuid.reserve( 16 + 4 );
	int i = 0;
	for ( auto c : _uuid )
	{
		char v1 = ( c >> 4 ) & 0x0F;
		char v2 = c & 0x0F;
		uuid.push_back( v1 < 10 ? v1 + '0' : v1 + 'A' - 10 );
		uuid.push_back( v2 < 10 ? v2 + '0' : v2 + 'A' - 10 );
		if ( i == 3 or i == 5 or i == 7 or i == 9 )
			uuid.push_back( '-' );
		++i;
	}
	return uuid;
}
}
