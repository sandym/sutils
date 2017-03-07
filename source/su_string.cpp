/*
 *  ustring.cpp
 *  sutils
 *
 *  Created by Sandy Martel on 2016/09/23.
 *  Copyright (c) 2016年 Sandy Martel. All rights reserved.
 *
 * Permission to use, copy, modify, distribute, and sell this software for any purpose is hereby
 * granted without fee. The sotware is provided "AS-IS" and without warranty of any kind, express,
 * implied or otherwise.
 */

#include "su_string.h"
#include "cfauto.h"
#include "su_flat_map.h"
#include <ciso646>
#include <stack>
#include <locale>
#include <cassert>
#include <cstring>
#include <cctype>
#include <codecvt>

namespace su
{

std::string to_string( const std::wstring &s )
{
    std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> conv;
    return conv.to_bytes( s );
}

std::string to_string( const std::u16string &s )
{
#if UPLATFORM_WIN
	static_assert(sizeof(wchar_t) == sizeof(char16_t), "");
	std::wstring w;
	w.reserve(s.size());
	for (auto it : s)
		w.push_back(it);
	return to_string( w );
#else
    std::wstring_convert<std::codecvt_utf8_utf16<char16_t>,char16_t> conversion;
    return conversion.to_bytes( s );
#endif
}

std::wstring to_wstring( const su::string_view &s )
{
    std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> conv;
    return conv.from_bytes( s.begin(), s.end() );
}

std::wstring to_wstring( const std::u16string &s )
{
	return to_wstring( to_string( s ) );
}

std::u16string to_u16string( const su::string_view &s )
{
#if UPLATFORM_WIN
	static_assert(sizeof(wchar_t) == sizeof(char16_t), "");
	auto w = to_wstring(s);
	std::u16string s16;
	s16.reserve(w.size());
	for (auto it : w)
		s16.push_back(it);
	return s16;
#else
	std::wstring_convert<std::codecvt_utf8_utf16<char16_t>,char16_t> conversion;
    return conversion.from_bytes( s.begin(), s.end() );
#endif
}

std::u16string to_u16string( const std::wstring &s )
{
	return to_u16string( to_string( s ) );
}

#if UPLATFORM_MAC || UPLATFORM_IOS
std::string to_string( CFStringRef i_cfstring )
{
	CFIndex len = CFStringGetLength( i_cfstring );
	CFStringInlineBuffer buffer;
	CFStringInitInlineBuffer( i_cfstring, &buffer, CFRangeMake( 0, len ) );
	std::u16string s16;
	for ( CFIndex i = 0; i < len; ++i )
		s16.push_back( CFStringGetCharacterFromInlineBuffer( &buffer, i ) );

	return to_string( s16 );
}

CFStringRef CreateCFString( const su::string_view &s )
{
	return CFStringCreateWithBytes( 0, (const UInt8 *)s.data(), s.length(), kCFStringEncodingUTF8, false );
}
#endif

std::string tolower( const su::string_view &s )
{
	std::string other( s.to_string() );
	for ( auto &it : other )
	{
		if ( it < 126 )
			it = (char)std::tolower( it );
	}
	return other;
}

std::string toupper( const su::string_view &s )
{
	std::string other( s.to_string() );
	for ( auto &it : other )
	{
		if ( it < 126 )
			it = (char)std::toupper( it );
	}
	return other;
}

int compare_nocase( const su::string_view &lhs, const su::string_view &rhs )
{
#if UPLATFORM_MAC || UPLATFORM_IOS
	cfauto<CFStringRef> lhsRef( CFStringCreateWithBytesNoCopy( 0, (const UInt8 *)lhs.data(), lhs.length(), kCFStringEncodingUTF8, false, kCFAllocatorNull ) );
	cfauto<CFStringRef> rhsRef( CFStringCreateWithBytesNoCopy( 0, (const UInt8 *)rhs.data(), rhs.length(), kCFStringEncodingUTF8, false, kCFAllocatorNull ) );
	return CFStringCompare( lhsRef, rhsRef, kCFCompareCaseInsensitive+kCFCompareNonliteral );
#elif UPLATFORM_WIN
	return _wcsicmp( to_wstring(lhs).c_str(), to_wstring(rhs).c_str() );
#else
	return wcscasecmp( to_wstring(lhs).c_str(), to_wstring(rhs).c_str() );
#endif
}

int ucompare( const su::string_view &lhs, const su::string_view &rhs )
{
#if UPLATFORM_MAC || UPLATFORM_IOS
	cfauto<CFStringRef> lhsRef( CFStringCreateWithBytesNoCopy( 0, (const UInt8 *)lhs.data(), lhs.length(), kCFStringEncodingUTF8, false, kCFAllocatorNull ) );
	cfauto<CFStringRef> rhsRef( CFStringCreateWithBytesNoCopy( 0, (const UInt8 *)rhs.data(), rhs.length(), kCFStringEncodingUTF8, false, kCFAllocatorNull ) );
	return CFStringCompare( lhsRef, rhsRef, kCFCompareLocalized+kCFCompareNonliteral );
#elif UPLATFORM_WIN
	return wcscoll( to_wstring(lhs).c_str(), to_wstring(rhs).c_str() );
#else
	return wcscmp( to_wstring(lhs).c_str(), to_wstring(rhs).c_str() );
#endif
}

int ucompare_nocase( const su::string_view &lhs, const su::string_view &rhs )
{
#if UPLATFORM_MAC || UPLATFORM_IOS
	cfauto<CFStringRef> lhsRef( CFStringCreateWithBytesNoCopy( 0, (const UInt8 *)lhs.data(), lhs.length(), kCFStringEncodingUTF8, false, kCFAllocatorNull ) );
	cfauto<CFStringRef> rhsRef( CFStringCreateWithBytesNoCopy( 0, (const UInt8 *)rhs.data(), rhs.length(), kCFStringEncodingUTF8, false, kCFAllocatorNull ) );
	return CFStringCompare( lhsRef, rhsRef, kCFCompareCaseInsensitive+kCFCompareLocalized+kCFCompareNonliteral );
#elif UPLATFORM_WIN
	return _wcsicoll( to_wstring(lhs).c_str(), to_wstring(rhs).c_str() );
#else
	return wcscasecmp( to_wstring(lhs).c_str(), to_wstring(rhs).c_str() );
#endif
}

int ucompare_numerically( const su::string_view &lhs, const su::string_view &rhs )
{
	ssize_t la = lhs.length(), lb = rhs.length();
	ssize_t shortest = std::min( la, lb );
	const char *pa = lhs.data();
	const char *pb = rhs.data();
	ssize_t i = 0;
	
	// skip common
	while ( i < shortest and pa[i] == pb[i] )
		++i;
	
	if ( (i == la or not std::isdigit(pa[i])) and (i == lb or not std::isdigit(pb[i])) )
	{
		if ( i == la )
			return int(i - lb);
		else if ( i == lb )
			return 1;
		else
			return pa[i] - pb[i];
	}
	else if ( i > 0 and std::isdigit( pa[i-1] ) )
	{
		do
		{
			--i;
		} while ( i > 0 and std::isdigit( pa[i-1] ) );
	}
	
	if ( i == shortest )
	{
		return int(la - lb);
	}
	else if ( not std::isdigit( pa[i] ) or not std::isdigit( pb[i] ) )
	{
		return pa[i] - pb[i];
	}
	else
	{
		pa += i;
		pb += i;
		return atoi( pa ) - atoi( pb );
	}
}

int ucompare_nocase_numerically( const su::string_view &lhs, const su::string_view &rhs )
{
	ssize_t la = lhs.length(), lb = rhs.length();
	ssize_t shortest = std::min( la, lb );
	const char *pa = lhs.data();
	const char *pb = rhs.data();
	ssize_t i = 0;
	
	// skip common
	while ( i < shortest and std::tolower( pa[i] ) == std::tolower( pb[i] ) )
		++i;
	
	if ( (i == la or not std::isdigit(pa[i])) and (i == lb or not std::isdigit(pb[i])) )
	{
		if ( i == la )
			return int(i - lb);
		else if ( i == lb )
			return 1;
		else
			return std::tolower( pa[i] ) - std::tolower( pb[i] );
	}
	else if ( i > 0 and std::isdigit( pa[i-1] ) )
	{
		do
		{
			--i;
		} while ( i > 0 and std::isdigit( pa[i-1] ) );
	}
	
	if ( i == shortest )
	{
		return int(la - lb);
	}
	else if ( not std::isdigit( pa[i] ) or not std::isdigit( pb[i] ) )
	{
		return std::tolower( pa[i] ) - std::tolower( pb[i] );
	}
	else
	{
		pa += i;
		pb += i;
		return atoi( pa ) - atoi( pb );
	}
}

su::string_view trimSpaces_view( const su::string_view &i_s )
{
	auto f = i_s.find_first_not_of( "\r\n\t " );
	auto l = i_s.find_last_not_of( "\r\n\t " );
	if ( f != su::string_view::npos )
	{
		assert( l != su::string_view::npos );
		return i_s.substr( f, l - f + 1 );
	}
	else
		return su::string_view();
}

//bool contains_nocase( const su::string_view &i_text, const su::string_view &i_s );
bool startsWith( const su::string_view &i_text, const su::string_view &i_s )
{
	if ( i_s.size() > i_text.size() )
		return false;
    return i_text.substr( 0, i_s.size() ) == i_s;
}
//bool startsWith_nocase( const su::string_view &i_text, const su::string_view &i_s );
//bool endsWith( const su::string_view &i_text, const su::string_view &i_s );
//bool endsWith_nocase( const su::string_view &i_text, const su::string_view &i_s );

std::string japaneseHiASCIIFix( const su::string_view &i_s )
{
	auto s16 = to_u16string( i_s );
	for ( auto &c : s16 )
	{
		if ( c >= 0xFF01 and c <= 0xFF5E )
			c = (c - 0xFF01 + '!');
	}
	return to_string( s16 );
}

}

namespace
{

const char16_t kICHI  = 0x4E00; // 1
const char16_t kNI    = 0x4E8C; // 2
const char16_t kSAN   = 0x4E09; // 3
const char16_t kYON   = 0x56DB; // 4
const char16_t kGO    = 0x4E94; // 5
const char16_t kROKU  = 0x516D; // 6
const char16_t kNANA  = 0x4E03; // 7
const char16_t kHACHI = 0x516B; // 8
const char16_t kKYU   = 0x4E5D; // 9
const char16_t kJUU   = 0x5341; // 10
const char16_t kHYAKU = 0x767E; // 100
const char16_t kSEN   = 0x5343; // 1000
const char16_t kMAN   = 0x4E07; // 10000

inline bool isKanjiDigit( char16_t c )
{
	return c == kICHI or
			c == kNI or
			c == kSAN or
			c == kYON or
			c == kGO or
			c == kROKU or
			c == kNANA or
			c == kHACHI or
			c == kKYU or
			c == kJUU or
			c == kHYAKU or
			c == kSEN or
			c == kMAN;
}

int kanjiNumberValue( std::u16string s )
{
	int factor, value = 0;
	
	// 10000
	auto p = s.find( kMAN );
	if ( p != std::u16string::npos )
	{
		if ( p == 0 )
			factor = 1;
		else
			factor = kanjiNumberValue( s.substr( 0, p ) );
		value += factor * 10000;
		s.erase( 0, p + 1 );
	}

	// 1000
	p = s.find( kSEN );
	if ( p != std::u16string::npos )
	{
		if ( p == 0 )
			factor = 1;
		else
		{
			factor = kanjiNumberValue( s.substr( 0, p ) );
			if ( factor >= 10 )
				throw std::out_of_range( "invalid kanji number" );
		}
		value += factor * 1000;
		s.erase( 0, p + 1 );
	}
	
	// 100
	p = s.find( kHYAKU );
	if ( p != std::u16string::npos )
	{
		if ( p == 0 )
			factor = 1;
		else
		{
			factor = kanjiNumberValue( s.substr( 0, p ) );
			if ( factor >= 10 )
				throw std::out_of_range( "invalid kanji number" );
		}
		value += factor * 100;
		s.erase( 0, p + 1 );
	}
	
	// 10
	p = s.find( kJUU );
	if ( p != std::u16string::npos )
	{
		if ( p == 0 )
			factor = 1;
		else
		{
			factor = kanjiNumberValue( s.substr( 0, p ) );
			if ( factor >= 10 )
				throw std::out_of_range( "invalid kanji number" );
		}
		value += factor * 10;
		s.erase( 0, p + 1 );
	}
	
	if ( not s.empty() )
	{
		if ( s.size() != 1 )
			throw std::out_of_range( "invalid kanji number" );
		switch ( s[0] )
		{
			case kICHI:  value += 1; break;
			case kNI:    value += 2; break;
			case kSAN:   value += 3; break;
			case kYON:   value += 4; break;
			case kGO:    value += 5; break;
			case kROKU:  value += 6; break;
			case kNANA:  value += 7; break;
			case kHACHI: value += 8; break;
			case kKYU:   value += 9; break;
			default: throw std::out_of_range( "invalid kanji number" );
		}
	}
	
	return value;
}

}

namespace su
{

std::string kanjiNumberFix( const su::string_view &i_s )
{
	auto s16 = to_u16string( i_s );
	std::u16string result;
	result.reserve( s16.size() );
	std::u16string::const_iterator kanjiStart = s16.end();
	for ( auto it = s16.cbegin(); it != s16.cend(); ++it )
	{
		if ( isKanjiDigit( *it ) )
		{
			if ( kanjiStart == s16.end() )
				kanjiStart = it;
		}
		else
		{
			if ( kanjiStart != s16.end() )
			{
				try
				{
					result.append(su::to_u16string(std::to_string(kanjiNumberValue( std::u16string( kanjiStart, it ) ))) );
				}
				catch ( ... )
				{
					result.append( kanjiStart, it );
				}
				kanjiStart = s16.end();
			}
			result.push_back( *it );
		}
	}
	if ( kanjiStart != s16.end() )
	{
		try
		{
			result.append(su::to_u16string(std::to_string(kanjiNumberValue( std::u16string( kanjiStart, s16.cend() ) ))) );
		}
		catch ( ... )
		{
			result.append( kanjiStart, s16.cend() );
		}
	}
	return to_string( result );
}

size_t levenshteinDistance( const su::string_view &i_string,
							const su::string_view &i_target )
{
	auto string16 = to_u16string( i_string );
	auto target16 = to_u16string( i_target );

	using su::u16string_view;

	struct key_t
	{
		key_t( const u16string_view &i_string, const u16string_view &i_target )
			: string( i_string ), target( i_target ){}
		
		u16string_view string, target;
		
		bool operator<( const key_t &k ) const
		{
			return std::memcmp( this, &k, sizeof( *this ) ) < 0;
		}
	};
	
	su::flat_map<key_t,size_t> mem;
	std::stack<key_t> st;
	
	st.emplace( string16, target16 );
	
	size_t dist = 0;
	for ( ;; )
	{
		key_t k = st.top();
		auto it = mem.find( k );
		if ( it != mem.end() )
		{
			st.pop();
			dist = it->second;
		}
		else
		{
			if ( k.string.empty() )
			{
				st.pop();
				dist = k.target.size();
			}
			else if ( k.target.empty() )
			{
				st.pop();
				dist = k.string.size();
			}
			else
			{
				key_t k1( k.string.substr( 1, k.string.size() - 1 ), k.target );
				key_t k2( k.string, k.target.substr( 1, k.target.size() - 1 ) );
				key_t k3( k.string.substr( 1, k.string.size() - 1 ), k.target.substr( 1, k.target.size() - 1 ) );
				auto d1 = mem.find( k1 );
				auto d2 = mem.find( k2 );
				auto d3 = mem.find( k3 );

				if ( d1 == mem.end() )
				{
					st.push( k1 );
					if ( d2 == mem.end() )
						st.push( k2 );
					if ( d3 == mem.end() )
						st.push( k3 );
					continue;
				}
				else if ( d2 == mem.end() )
				{
					st.push( k2 );
					if ( d3 == mem.end() )
						st.push( k3 );
					continue;
				}
				else if ( d3 == mem.end() )
				{
					st.push( k3 );
					continue;
				}
				else
				{
					size_t cost = k.string.front() == k.target.front() ? 0 : 1;
					dist = std::min( d1->second + 1, std::min( d2->second + 1, d3->second + cost ) );
					st.pop();
				}
			}
		}
		if ( not st.empty() )
			mem[k] = dist;
		else
			break;
	}
	return dist;
}

}
