/*
 *  version.h
 *  sutils
 *
 *  Created by Sandy Martel on 2016/02/09.
 *  Copyright (c) 2016年 Sandy Martel. All rights reserved.
 *
 * Permission to use, copy, modify, distribute, and sell this software for any
 * purpose is hereby granted without fee. The sotware is provided "AS-IS" and
 * without warranty of any kind, express, implied or otherwise.
 */

#ifndef H_SU_VERSION
#define H_SU_VERSION

#if __has_include("user_config.h")
#include "user_config.h"
#endif

#ifndef PRODUCT_VERSION
#	define PRODUCT_VERSION 0.0.0.1
#endif

// helper macros
#define _STRINGIFY(x) #x
#define STRINGIFY(x) _STRINGIFY(x)

#ifdef __cplusplus

#include <ciso646>
#include <string>
#include <string_view>

#ifdef minor
#undef minor
#endif
#ifdef major
#undef major
#endif

namespace su {

class version final
{
public:
	static version from_string( const std::string_view &i_version );
	
	version( int i_major, int i_minor = 0, int i_patch = 0, int i_build = 1 )
		: _major( i_major ),
		  _minor( i_minor ),
		  _patch( i_patch ),
		  _build( i_build )
	{
	}

	int major() const { return _major; }
	int minor() const { return _minor; }
	int patch() const { return _patch; }
	int build() const { return _build; }

	bool operator<( const version &rhs ) const { return cmp( rhs ) < 0; }
	bool operator>( const version &rhs ) const { return cmp( rhs ) > 0; }
	bool operator==( const version &rhs ) const { return cmp( rhs ) == 0; }
	bool operator>=( const version &rhs ) const { return cmp( rhs ) >= 0; }
	bool operator!=( const version &rhs ) const { return cmp( rhs ) != 0; }
	bool operator<=( const version &rhs ) const { return cmp( rhs ) <= 0; }

	std::string string() const; //!< 1.2.3
	std::string full_string() const; //!< 1.2.3.4, same as string() + build number

private:
	const int _major = 0;
	const int _minor = 0;
	const int _patch = 0;
	const int _build = 1;
	
	int cmp( const version &rhs ) const
	{
		auto d = _major - rhs._major;
		if ( d != 0 )
			return d;
		d = _minor - rhs._minor;
		if ( d != 0 )
			return d;
		d = _patch - rhs._patch;
		if ( d != 0 )
			return d;
		return _build - rhs._build;
	}
};

inline version CURRENT_VERSION()
{
	static auto s_version = version::from_string( STRINGIFY(PRODUCT_VERSION) );
	return s_version;
}

inline const char *build_revision()
{
#ifdef GIT_REVISION
	return "#" STRINGIFY(GIT_REVISION);
#else
	return "";
#endif
}
inline const char *build_date() { return __DATE__ " " __TIME__; }

}

inline std::string to_string( const su::version &v ) { return v.full_string(); }

#endif

#define PRODUCT_VERSION_STRING STRINGIFY(PRODUCT_VERSION)

#endif
