/*
 *  version.cpp
 *  sutils
 *
 *  Created by Sandy Martel on 2016/02/09.
 *  Copyright (c) 2016年 Sandy Martel. All rights reserved.
 *
 * Permission to use, copy, modify, distribute, and sell this software for any
 * purpose is hereby  granted without fee. The sotware is provided "AS-IS" and
 * without warranty of any kind, express, implied or otherwise.
 */

#include "su/miscs/version.h"
#include <cctype>
#include <tuple>
#include "su/base/platform.h"

namespace {

inline bool is_separator( char c )
{
	return c == '.' or c == ',' or c == '-' or c == '_';
}

}

namespace su {

version version::from_string( const std::string_view &i_version )
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
		if ( it == i_version.end() or not is_separator( *it ) )
			break;
		++it;
	}

	// extract major.minor.patch.buildNumber
	int major = 0, minor = 0, patch = 0, buildNumber = 1;
	if ( i > 0 )
		major = comp[0];
	if ( i > 1 )
		minor = comp[1];
	if ( i > 2 )
		patch = comp[2];
	if ( i > 3 )
		buildNumber = comp[3];
	
	return version( major, minor, patch, buildNumber );
}

std::string version::string() const
{
	auto s = std::to_string( _major );
	s += "." + std::to_string( _minor );
	s += "." + std::to_string( _patch );
 	return s;
}

std::string version::full_string() const
{
	auto s = std::to_string( _major );
	s += "." + std::to_string( _minor );
	s += "." + std::to_string( _patch );
	s += "." + std::to_string( _build );
 	return s;
}

std::string build_revision()
{
	std::string rev;
#ifdef SVN_REVISION
	rev += "svn(#" STRINGIFY(SVN_REVISION) ")";
#endif
#ifdef GIT_REVISION
	rev += "git(#" STRINGIFY(GIT_REVISION) ")";
#endif
	return rev;
}

}

