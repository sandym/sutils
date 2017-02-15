/*
 *  su_string.h
 *  sutils
 *
 *  Created by Sandy Martel on 2016/09/23.
 *  Copyright (c) 2016年 Sandy Martel. All rights reserved.
 *
 * Permission to use, copy, modify, distribute, and sell this software for any purpose is hereby
 * granted without fee. The sotware is provided "AS-IS" and without warranty of any kind, express,
 * implied or otherwise.
 */

#ifndef H_SU_STRING
#define H_SU_STRING

#include <string>
#include <vector>
#include "su_shim/string_view.h"
#include "su_platform.h"

#if UPLATFORM_MAC || UPLATFORM_IOS
#include <CoreFoundation/CoreFoundation.h>
#endif

namespace su
{

// conversion
std::string to_string( const std::wstring &s );
std::string to_string( const std::u16string &s );
std::wstring to_wstring( const su::string_view &s );
std::wstring to_wstring( const std::u16string &s );
std::u16string to_u16string( const su::string_view &s );
std::u16string to_u16string( const std::wstring &s );

#if UPLATFORM_MAC || UPLATFORM_IOS
std::string to_string( CFStringRef i_cfstring );
CFStringRef CreateCFString( const su::string_view &s );
#endif

std::string tolower( const su::string_view &s );
std::string toupper( const su::string_view &s );

/*!
	@brief Case insensitive compare.

		Compare 2 string, case insensitive. Take care of decomposed and precomposed Unicode..
	@param lhs string to compare
	@param rhs string to compare to
	@return     like strcmp()
*/
int compare_nocase( const su::string_view &lhs, const su::string_view &rhs );

/*!
   @brief compare 2 string using the current locale comparaison.

   @param[in]     lhs utf-8 string
   @param[in]     rhs utf-8 string
   @return     same as strcmp
*/
int ucompare( const su::string_view &lhs, const su::string_view &rhs );

/*!
   @brief compare 2 string using the current locale comparaison but ignoring case.

   @param[in]     lhs utf-8 string
   @param[in]     rhs utf-8 string
   @return     same as strcmp
*/
int ucompare_nocase( const su::string_view &lhs, const su::string_view &rhs );

/*!
   @brief compare 2 string numerically.

	   That is a1 < a5 < a10.
   @param[in] lhs utf-8 string
   @param[in] rhs utf-8 string
   @return     same as strcmp
*/
int ucompare_numerically( const su::string_view &lhs, const su::string_view &rhs );

/*!
   @brief compare 2 string numerically but ignoring case.

	   That is A1 < a5 < A10.
   @param[in] lhs utf-8 string
   @param[in] rhs utf-8 string
   @return     same as strcmp
*/
int ucompare_nocase_numerically( const su::string_view &lhs, const su::string_view &rhs );


template<typename CHAR_TYPE>
std::vector<su::basic_string_view<CHAR_TYPE>> split_view( const su::basic_string_view<CHAR_TYPE> &s, CHAR_TYPE c )
{
	std::vector<su::basic_string_view<CHAR_TYPE>> res;
	typename su::basic_string_view<CHAR_TYPE>::size_type a = 0;
	for ( typename su::basic_string_view<CHAR_TYPE>::size_type i = 0; i < s.length(); ++i )
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

template<typename CHAR_TYPE>
std::vector<std::basic_string<CHAR_TYPE>> split( const su::basic_string_view<CHAR_TYPE> &s, CHAR_TYPE c )
{
	auto viewList = split_view( s, c );
	std::vector<std::basic_string<CHAR_TYPE>> res;
	res.reserve( viewList.size() );
	for ( auto it : viewList )
		res.push_back( it.to_string() );
	return res;
}

template<typename CHAR_TYPE, typename COND>
std::vector<su::basic_string_view<CHAR_TYPE>> split_view_if( const su::basic_string_view<CHAR_TYPE> &s, COND i_cond )
{
	std::vector<su::basic_string_view<CHAR_TYPE>> res;
	typename su::basic_string_view<CHAR_TYPE>::size_type a = 0;
	for ( typename su::basic_string_view<CHAR_TYPE>::size_type i = 0; i < s.length(); ++i )
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

template<typename CHAR_TYPE, typename COND>
std::vector<std::basic_string<CHAR_TYPE>> split_if( const su::basic_string_view<CHAR_TYPE> &s, COND i_cond )
{
	auto viewList = split_view_if( s, i_cond );
	std::vector<std::basic_string<CHAR_TYPE>> res;
	res.reserve( viewList.size() );
	for ( auto it : viewList )
		res.push_back( it.to_string() );
	return res;
}

template<class STRING_TYPE,class CONT>
STRING_TYPE join( const CONT &i_list, typename STRING_TYPE::value_type sep )
{
	STRING_TYPE result;
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
			result.append( 1, sep );
			result.append( *it );
			++it;
		}
	}
	return result;
}

template<class STRING_TYPE,class CONT>
STRING_TYPE join( const CONT &i_list, const STRING_TYPE &i_sep )
{
	STRING_TYPE result;
	size_t len = 0;
	for ( auto it : i_list )
		len += it.size();
	result.reserve( len + i_list.size() + i_sep.size() );
	auto it = i_list.begin();
	if ( it != i_list.end() )
	{
		result.append( *it );
		++it;
		while ( it != i_list.end() )
		{
			result.append( i_sep );
			result.append( *it );
			++it;
		}
	}
	return result;
}

/*!
   @brief Remove spaces at the begining and end of the string.
*/
su::string_view trimSpaces_view( const su::string_view &i_s );
inline std::string trimSpaces( const su::string_view &i_s ) { return trimSpaces_view( i_s ).to_string(); }

inline bool contains( const su::string_view &i_text, const su::string_view &i_s )
{
	return i_text.find( i_s ) != su::string_view::npos;
}

bool contains_nocase( const su::string_view &i_text, const su::string_view &i_s );
bool startsWith( const su::string_view &i_text, const su::string_view &i_s );
bool startsWith_nocase( const su::string_view &i_text, const su::string_view &i_s );
bool endsWith( const su::string_view &i_text, const su::string_view &i_s );
bool endsWith_nocase( const su::string_view &i_text, const su::string_view &i_s );

/*!
   @brief replace monospaced japanese ASCII.
*/
std::string japaneseHiASCIIFix( const su::string_view &i_s );

/*!
   @brief replace kanji numbers with arab ones.
*/
std::string kanjiNumberFix( const su::string_view &i_s );

size_t levenshteinDistance( const su::string_view &i_string,
							const su::string_view &i_target );

}

#endif
