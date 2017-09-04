/*
 *  su_version.h
 *  sutils
 *
 *  Created by Sandy Martel on 2016/02/09.
 *  Copyright (c) 2016年 Sandy Martel. All rights reserved.
 *
 * Permission to use, copy, modify, distribute, and sell this software for any purpose is hereby
 * granted without fee. The sotware is provided "AS-IS" and without warranty of any kind, express,
 * implied or otherwise.
 */

#ifndef H_SU_VERSION
#define H_SU_VERSION

#ifndef PRODUCT_VERSION_FULL
#define PRODUCT_VERSION_FULL 0,0,1,1
#endif

#ifdef __cplusplus

#include <ciso646>
#include <string>
#include <string_view>

namespace su {

class version final
{
public:
	static version from_string( const std::string_view &i_version );
	constexpr version( int i_major, int i_minor = 0, int i_patch = 0, int i_build = 0 )
		: _major( i_major ),
		  _minor( i_minor ),
		  _patch( i_patch ),
		  _build( i_build )
	{
	}

	constexpr int major() const { return _major; }
	constexpr int minor() const { return _minor; }
	constexpr int patch() const { return _patch; }
	constexpr int build() const { return _build; }

	constexpr bool operator<( const version &rhs ) const { return cmp( rhs ) < 0; }
	constexpr bool operator>( const version &rhs ) const { return cmp( rhs ) > 0; }
	constexpr bool operator==( const version &rhs ) const { return cmp( rhs ) == 0; }
	constexpr bool operator>=( const version &rhs ) const { return cmp( rhs ) >= 0; }
	constexpr bool operator!=( const version &rhs ) const { return cmp( rhs ) != 0; }
	constexpr bool operator<=( const version &rhs ) const { return cmp( rhs ) <= 0; }

	std::string string() const; //!< 1.2.3
	std::string full_string() const; //!< 1.2.3.4, same as string() + build number

private:
	const int _major = 0;
	const int _minor = 0;
	const int _patch = 0;
	const int _build = 0;
	
	constexpr int cmp( const version &rhs ) const
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

constexpr version CURRENT_VERSION() { return version(PRODUCT_VERSION_FULL); }

std::string build_revision();
constexpr const char *build_date() { return __DATE__ " " __TIME__; }

}

inline std::string to_string( const su::version &v ) { return v.full_string(); }

#endif

// helper macros
#define _STRINGIFY(x) #x
#define STRINGIFY(x) _STRINGIFY(x)

#define _SELECT_0(a0,...) a0
#define SELECT_0(x) _SELECT_0(x)
#define _SELECT_1(a0,a1,...) a1
#define SELECT_1(x) _SELECT_1(x)
#define _SELECT_2(a0,a1,a2,...) a2
#define SELECT_2(x) _SELECT_2(x)
#define _SELECT_3(a0,a1,a2,a3) a3
#define SELECT_3(x) _SELECT_3(x)

// macro version
#ifndef PRODUCT_VERSION_MAJOR
#	define PRODUCT_VERSION_MAJOR SELECT_0(PRODUCT_VERSION_FULL)
#endif
#ifndef PRODUCT_VERSION_MINOR
#	define PRODUCT_VERSION_MINOR SELECT_1(PRODUCT_VERSION_FULL)
#endif
#ifndef PRODUCT_VERSION_PATCH
#	define PRODUCT_VERSION_PATCH SELECT_2(PRODUCT_VERSION_FULL)
#endif
#ifndef PRODUCT_VERSION_BUILD
#	define PRODUCT_VERSION_BUILD SELECT_3(PRODUCT_VERSION_FULL)
#endif

#ifndef PRODUCT_VERSION
#	define PRODUCT_VERSION SELECT_0(PRODUCT_VERSION_FULL).SELECT_2(PRODUCT_VERSION_FULL).SELECT_2(PRODUCT_VERSION_FULL)
#endif
#ifndef PRODUCT_VERSION_STRING
#	define PRODUCT_VERSION_STRING STRINGIFY(PRODUCT_VERSION)
#endif

#endif
