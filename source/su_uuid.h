/*
 *  su_uuid.h
 *  sutils
 *
 *  Created by Sandy Martel on 05-05-07.
 *  Copyright (c) 2015年 Sandy Martel. All rights reserved.
 *
 * Permission to use, copy, modify, distribute, and sell this software for any purpose is hereby
 * granted without fee. The sotware is provided "AS-IS" and without warranty of any kind, express,
 * implied or otherwise.
 */

#ifndef H_SU_UUID
#define H_SU_UUID

#include <array>
#include <string>

namespace su {
/*!
   @brief uuid value, unique across the universe.
*/
class uuid final
{
public:
	static uuid create();

	uuid() = default;
	uuid( const uuid & ) = default;
	uuid( uuid && ) = default;
	uuid( const std::array<uint8_t, 16> &i_value );
	uuid( const std::string &i_value );

	uuid &operator=( const uuid & ) = default;
	uuid &operator=( uuid && ) = default;

	inline bool operator==( const uuid &i_other ) const { return _uuid == i_other._uuid; }
	inline bool operator!=( const uuid &i_other ) const { return _uuid != i_other._uuid; }
	inline bool operator<( const uuid &i_other ) const { return _uuid < i_other._uuid; }
	inline const std::array<uint8_t, 16> &value() const { return _uuid; }
	std::string string() const;

private:
	std::array<uint8_t, 16> _uuid;
};

}

inline std::string to_string( const su::uuid &v ) { return v.string(); }

#endif
