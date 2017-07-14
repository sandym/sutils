/*
 *  su_usersettings.h
 *  sutils
 *
 *  Created by Sandy Martel on 08-03-27.
 *  Copyright (c) 2015年 Sandy Martel. All rights reserved.
 *
 * Permission to use, copy, modify, distribute, and sell this software for any purpose is hereby
 * granted without fee. The sotware is provided "AS-IS" and without warranty of any kind, express,
 * implied or otherwise.
 */

#ifndef H_SU_USERSETTINGS
#define H_SU_USERSETTINGS

#include "su_platform.h"
#include "su_shim/string_view.h"

#if UPLATFORM_MAC || UPLATFORM_IOS
#include <CoreFoundation/CoreFoundation.h>
#include "cfauto.h"
#endif

namespace su {

/*!
   @brief cross platform User Settings API.
*/
class usersettings
{
public:
	static void init( const su::string_view &i_company, const su::string_view &i_application );
	
	usersettings();
	~usersettings();
	
	template<typename T>
	T read( const su::string_view &i_name, const T &i_default = {} );
	
	template<typename T>
	void write( const su::string_view &i_name, const T &i_value );
	
	void write( const su::string_view &i_name, const char *i_value );
	
private:
#if UPLATFORM_WIN
	HKEY _prefsKey;
#elif UPLATFORM_MAC || UPLATFORM_IOS
	cfauto<CFStringRef> _prefsKey;
#endif
};

}

#endif
