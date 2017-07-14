/*
 *  su_version.cpp
 *  sutils
 *
 *  Created by Sandy Martel on 2016/02/09.
 *  Copyright (c) 2016年 Sandy Martel. All rights reserved.
 *
 * Permission to use, copy, modify, distribute, and sell this software for any purpose is hereby
 * granted without fee. The sotware is provided "AS-IS" and without warranty of any kind, express,
 * implied or otherwise.
 */

#include "su_version.h"
#include <cctype>
#include <tuple>
#include "su_platform.h"

namespace {

inline bool isSeparator( char c )
{
	return c == '.' or c == ',' or c == '-' or c == '_';
}

}

namespace su {

version::version( const su_std::string_view &i_version )
{
	auto it = i_version.begin();

	// skip to first digit
	while ( it != i_version.end() and not std::isdigit( *it ) )
		++it;

	int comp[4];
	int i = 0;
	while ( it != i_version.end() and std::isdigit( *it ) )
	{
		// digit
		int v = 0;
		while ( it != i_version.end() and std::isdigit( *it ) )
		{
			v = (v*10) + (*it - '0');
			++it;
		}
		comp[i] = v;
		if ( ++i >= 4 )
			break;

		// expect a separator
		if ( it == i_version.end() or not isSeparator( *it ) )
			break;
		++it;
	}

	// extract major.minor.patch.buildNumber
	if ( i > 0 )
		_major = comp[0];
	if ( i > 1 )
		_minor = comp[1];
	if ( i > 2 )
		_patch = comp[2];
	if ( i > 3 )
		_buildNumber = comp[3];
}

version::version( int i_major, int i_minor, int i_patch, int i_buildNumber )
	: _major( i_major ),
	  _minor( i_minor ),
	  _patch( i_patch ),
	  _buildNumber( i_buildNumber )
{
}

#define _STRINGIFY(x) #x
#define STRINGIFY(x) _STRINGIFY(x)

std::string version::revision() const
{
	std::string rev;
	if ( this == &CURRENT_VERSION )
	{
#ifdef GIT_REVISION
		rev += "git(#" STRINGIFY(GIT_REVISION) ")";
#endif
#ifdef SVN_REVISION
		rev += "svn(#" STRINGIFY(SVN_REVISION) ")";
#endif
	}
	return rev;
}

std::string version::build_date() const
{
	if ( this == &CURRENT_VERSION )
		return __DATE__ " " __TIME__;
	return std::string();
}

std::string version::string() const
{
	return std::to_string( _major ) + "." +
			std::to_string( _minor ) + "." +
			std::to_string( _patch ) + "." +
			std::to_string( _buildNumber )
#ifndef NDEBUG
			+ "(debug)"
#endif
			;
}

std::string version::string_platform() const
{
#if UPLATFORM_MAC
#	define PLATFORM_NAME     "/mac"
#elif UPLATFORM_WIN
#   define PLATFORM_NAME     "/win"
#elif UPLATFORM_IOS
#   define PLATFORM_NAME     "/ios"
#elif UPLATFORM_ANDROID
#   define PLATFORM_NAME     "/android"
#elif UPLATFORM_LINUX
#   define PLATFORM_NAME     "/linux"
#elif UPLATFORM_BSD
#   define PLATFORM_NAME     "/bsd"
#else
#   error "Unsupported OS"
#endif
	return string() + PLATFORM_NAME;
}

std::string version::full() const
{
	auto vers = string_platform();
	auto date = build_date();
	if ( not date.empty() )
		vers += "\nbuilt: " + date;
	auto rev = revision();
	if ( not rev.empty() )
		vers += "\nrev: " + rev;
	
	return vers;
}

bool version::operator<( const version &i_other ) const
{
	return std::tie( _major, _minor, _patch, _buildNumber ) < std::tie( i_other._major, i_other._minor, i_other._patch, i_other._buildNumber );
}

bool version::operator==( const version &i_other ) const
{
	return std::tie( _major, _minor, _patch, _buildNumber ) == std::tie( i_other._major, i_other._minor, i_other._patch, i_other._buildNumber );
}

bool version::operator>( const version &i_other ) const
{
	return std::tie( _major, _minor, _patch, _buildNumber ) > std::tie( i_other._major, i_other._minor, i_other._patch, i_other._buildNumber );
}

#ifndef BUILD_VERSION
#define BUILD_VERSION 0,0,1,0
#endif

const version CURRENT_VERSION( BUILD_VERSION );

}
