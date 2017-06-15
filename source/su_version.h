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

#include <ciso646>
#include <string>
#include "su_shim/string_view.h"

namespace su {

class version final
{
public:
	version() = default;
	version( const su::string_view &i_version );
	version( int i_major, int i_minor = 0, int i_patch = 0, int i_buildNumber = 0 );

	inline int major() const { return _major; }
	inline int minor() const { return _minor; }
	inline int patch() const { return _patch; }
	inline int buildNumber() const { return _buildNumber; }

	std::string revision() const;
	std::string build_date() const;

	std::string string() const;	//!< major.minor.patch.buildNumber(data)
	std::string string_platform() const;
	std::string full() const;

	bool operator<( const version &i_other ) const;
	bool operator>( const version &i_other ) const;
	bool operator==( const version &i_other ) const;

	inline bool operator>=( const version &i_other ) const
	{
		return not operator<( i_other );
	}
	inline bool operator!=( const version &i_other ) const
	{
		return not operator==( i_other );
	}
	inline bool operator<=( const version &i_other ) const
	{
		return not operator>( i_other );
	}

private:
	int _major = 0;
	int _minor = 0;
	int _patch = 0;
	int _buildNumber = 0;
};

extern const version CURRENT_VERSION;

}

#endif
