/*
 *  su_string_load.h
 *  sutils
 *
 *  Created by Sandy Martel on 06-09-15.
 *  Copyright (c) 2015年 Sandy Martel. All rights reserved.
 *
 * Permission to use, copy, modify, distribute, and sell this software for any purpose is hereby
 * granted without fee. The sotware is provided "AS-IS" and without warranty of any kind, express,
 * implied or otherwise.
 */

#ifndef H_SU_STRINGLOAD
#define H_SU_STRINGLOAD

#include "shim/string_view.h"

namespace su {

/*!
   @function ustring_load
   @abstract Load a localized string from a given string table in resources.
   @discussion Load a string form the given string table. It return the key in case of errors..
   @param i_key   The key.
   @param i_table The string table name.
   @result     The (hopefully) translated string.
*/
std::string string_load( const su::string_view &i_key, const su::string_view &i_table );

}

#endif
