/*
 *  su_string_utils.h
 *  sutils
 *
 *  Created by Sandy Martel on 2016/09/23.
 *  Copyright (c) 2016年 Sandy Martel. All rights reserved.
 *
 * Permission to use, copy, modify, distribute, and sell this software for any
 * purpose is hereby granted without fee. The sotware is provided "AS-IS" and
 * without warranty of any kind, express, implied or otherwise.
 */

#ifndef H_SU_STRING_UTILS
#define H_SU_STRING_UTILS

#include <string>
#include <vector>
#include <string_view>
#include "su_platform.h"

#if UPLATFORM_MAC || UPLATFORM_IOS
#include <CoreFoundation/CoreFoundation.h>
#endif

namespace su {

// conversion
std::string to_string( const std::wstring_view &s );
std::string to_string( const std::u16string_view &s );
std::wstring to_wstring( const std::string_view &s );
std::wstring to_wstring( const std::u16string_view &s );
std::u16string to_u16string( const std::string_view &s );
std::u16string to_u16string( const std::wstring_view &s );

#if UPLATFORM_MAC || UPLATFORM_IOS
std::string to_string( CFStringRef i_cfstring );
CFStringRef CreateCFString( const std::string_view &s );
#endif

std::string tolower( const std::string_view &s );
std::string toupper( const std::string_view &s );

/*!
	@brief Case insensitive compare.

		Compare 2 string, case insensitive. Take care of decomposed and
		precomposed Unicode..
	@param lhs string to compare
	@param rhs string to compare to
	@return     like strcmp()
*/
int compare_nocase( const std::string_view &lhs, const std::string_view &rhs );

/*!
   @brief compare 2 string using the current locale comparaison.

   @param[in]     lhs utf-8 string
   @param[in]     rhs utf-8 string
   @return     same as strcmp
*/
int ucompare( const std::string_view &lhs, const std::string_view &rhs );

/*!
   @brief compare 2 string using the current locale comparaison but ignoring case.

   @param[in]     lhs utf-8 string
   @param[in]     rhs utf-8 string
   @return     same as strcmp
*/
int ucompare_nocase( const std::string_view &lhs, const std::string_view &rhs );

/*!
   @brief compare 2 string numerically.

	   That is a1 < a5 < a10.
   @param[in] lhs utf-8 string
   @param[in] rhs utf-8 string
   @return     same as strcmp
*/
int ucompare_numerically( const std::string_view &lhs, const std::string_view &rhs );

/*!
   @brief compare 2 string numerically but ignoring case.

	   That is A1 < a5 < A10.
   @param[in] lhs utf-8 string
   @param[in] rhs utf-8 string
   @return     same as strcmp
*/
int ucompare_nocase_numerically( const std::string_view &lhs, const std::string_view &rhs );

template<typename T>
std::vector<T> split( const T &s, typename T::value_type c )
{
	std::vector<T> res;
	typename T::size_type a = 0;
	for ( typename T::size_type i = 0; i < s.length(); ++i )
	{
		if ( s[i] == c )
		{
			if ( (i - a) > 0 )
				res.push_back( s.substr( a, i - a ) );
			a = i + 1;
		}
	}
	if ( (s.length()-a) > 0 )
		res.push_back( s.substr( a, s.length() - a ) );
	return res;
}

template<typename T, typename COND>
std::vector<T>
	split_if( const T &s, COND i_cond )
{
	std::vector<T> res;
	typename T::size_type a = 0;
	for ( typename T::size_type i = 0; i < s.length(); ++i )
	{
		if ( i_cond( s[i] ) )
		{
			if ( (i - a) > 0 )
				res.push_back( s.substr( a, i - a ) );
			a = i + 1;
		}
	}
	if ( (s.length()-a) > 0 )
		res.push_back( s.substr( a, s.length() - a ) );
	return res;
}

template<typename T,typename CONT,typename SEP>
T join( const CONT &i_list, const SEP &sep )
{
	T result;
	size_t len = 0;
	for ( auto it : i_list )
		len += it.size();
	result.reserve( len + i_list.size() );
	auto it = i_list.begin();
	if ( it != i_list.end() )
	{
		result.append( *it );
		++it;
		while ( it != i_list.end() )
		{
			result += sep;
			result.append( *it );
			++it;
		}
	}
	return result;
}

/*!
   @brief Remove spaces at the begining and end of the string.
*/
std::string_view trim_spaces_view( const std::string_view &i_s );
inline std::string trim_spaces( const std::string_view &i_s )
{
	return std::string{ trim_spaces_view( i_s ) };
}

inline bool contains( const std::string_view &i_text, const std::string_view &i_s )
{
	return i_text.find( i_s ) != std::string_view::npos;
}

bool contains_nocase( const std::string_view &i_text, const std::string_view &i_s );
bool starts_with( const std::string_view &i_text, const std::string_view &i_s );
bool starts_with_nocase( const std::string_view &i_text, const std::string_view &i_s );
bool ends_with( const std::string_view &i_text, const std::string_view &i_s );
bool ends_with_nocase( const std::string_view &i_text, const std::string_view &i_s );

/*!
   @brief replace monospaced japanese ASCII.
*/
std::string japanese_hiASCII_fix( const std::string_view &i_s );

/*!
   @brief replace kanji numbers with arab ones.
*/
std::string kanjiNumberFix( const std::string_view &i_s );

size_t levenshtein_distance( const std::string_view &i_string,
							const std::string_view &i_target );

}

#endif
