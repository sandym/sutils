/*
 *  su_string_utils.cpp
 *  sutils
 *
 *  Created by Sandy Martel on 2016/09/23.
 *  Copyright (c) 2016年 Sandy Martel. All rights reserved.
 *
 * Permission to use, copy, modify, distribute, and sell this software for any
 * purpose is hereby granted without fee. The sotware is provided "AS-IS" and
 * without warranty of any kind, express, implied or otherwise.
 */

#include "su_string_utils.h"
#include "su_flat_map.h"
#include "ConvertUTF.h"
#include <stack>
#include <cassert>
#include <cstring>
#include <cctype>
#include <ciso646>

#if UPLATFORM_MAC || UPLATFORM_IOS
#include "cfauto.h"
#elif UPLATFORM_UNIX
#include "unicode/uchar.h"
#include "unicode/unistr.h"
#elif UPLATFORM_WIN
#include <Windows.h>
#include <Winuser.h>
#undef OUT
#undef IN
#undef min
#endif

namespace {

template<int IN_SIZE,int OUT_SIZE>
struct utf_type_traits;

template<>
struct utf_type_traits<4,1>
{
	using input_char_type = UTF32;
	using output_char_type = UTF8;
	constexpr static auto convert = ConvertUTF32toUTF8;
};
template<>
struct utf_type_traits<2,1>
{
	using input_char_type = UTF16;
	using output_char_type = UTF8;
	constexpr static auto convert = ConvertUTF16toUTF8;
};
template<>
struct utf_type_traits<1,4>
{
	using input_char_type = UTF8;
	using output_char_type = UTF32;
	constexpr static auto convert = ConvertUTF8toUTF32;
};
template<>
struct utf_type_traits<2,4>
{
	using input_char_type = UTF16;
	using output_char_type = UTF32;
	constexpr static auto convert = ConvertUTF16toUTF32;
};
template<>
struct utf_type_traits<1,2>
{
	using input_char_type = UTF8;
	using output_char_type = UTF16;
	constexpr static auto convert = ConvertUTF8toUTF16;
};
template<>
struct utf_type_traits<4,2>
{
	using input_char_type = UTF32;
	using output_char_type = UTF16;
	constexpr static auto convert = ConvertUTF32toUTF16;
};

template<typename IN,
			typename OUT,
			int IN_SIZE=sizeof(typename IN::value_type),
			int OUT_SIZE=sizeof(typename OUT::value_type)>
struct convert_utf
{
	static inline OUT convert( const IN &i_s )
	{
		using tt = utf_type_traits<IN_SIZE,OUT_SIZE>;
		
		OUT s;
		typename tt::output_char_type buf[512];
		auto src = reinterpret_cast<const typename tt::input_char_type *>( i_s.data() );
		auto srcEnd = src + i_s.size();
		typename tt::output_char_type *targetStart = buf;
		while ( src < srcEnd )
		{
			// Convert a block. Just bail out if we get an error.
			auto result = tt::convert( &src, srcEnd, &targetStart, buf + 512, strictConversion );
			if ( result == sourceIllegal or result == sourceExhausted )
				break;
			s.append( reinterpret_cast<const typename OUT::value_type *>( buf ), targetStart - buf );
			targetStart = buf;
		}
		return s;
	}
};
template<typename IN, typename OUT>
struct convert_utf<IN,OUT,2,2>
{
	static inline OUT convert(const IN &i_s)
	{
		OUT s( reinterpret_cast<const typename OUT::value_type *>( i_s.data() ), i_s.size() );
		return s;
	}
};

}

namespace su {

std::string to_string( const std::wstring_view &s )
{
    return convert_utf<std::wstring_view,std::string>::convert( s );
}
std::string to_string( const std::u16string_view &s )
{
    return convert_utf<std::u16string_view,std::string>::convert( s );
}
std::wstring to_wstring( const std::string_view &s )
{
    return convert_utf<std::string_view,std::wstring>::convert( s );
}
std::wstring to_wstring( const std::u16string_view &s )
{
    return convert_utf<std::u16string_view,std::wstring>::convert( s );
}
std::u16string to_u16string( const std::string_view &s )
{
    return convert_utf<std::string_view,std::u16string>::convert( s );
}
std::u16string to_u16string( const std::wstring_view &s )
{
    return convert_utf<std::wstring_view,std::u16string>::convert( s );
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
CFStringRef CreateCFString( const std::string_view &s )
{
	return CFStringCreateWithBytes( 0, (const UInt8 *)s.data(), s.length(),
												kCFStringEncodingUTF8, false );
}
#endif

std::string tolower( const std::string_view &s )
{
#if UPLATFORM_MAC || UPLATFORM_IOS
	cfauto<CFStringRef> cfs( CFStringCreateWithBytesNoCopy( 0, (const UInt8 *)s.data(), s.size(), kCFStringEncodingUTF8, false, kCFAllocatorNull ) );
	cfauto<CFMutableStringRef> ms( CFStringCreateMutableCopy( 0, 0, cfs ) );
	CFStringLowercase( ms, nullptr );
	return to_string( ms );
#elif UPLATFORM_WIN
	auto w = to_wstring(s);
	CharLowerW(w.data());
	return to_string(w);
#else
	auto us = icu::UnicodeString::fromUTF8( icu::StringPiece( s.data(), s.size() ) );
	std::string s8;
	return us.toLower().toUTF8String(s8);
#endif
}

std::string toupper( const std::string_view &s )
{
#if UPLATFORM_MAC || UPLATFORM_IOS
	cfauto<CFStringRef> cfs( CFStringCreateWithBytesNoCopy( 0, (const UInt8 *)s.data(), s.size(), kCFStringEncodingUTF8, false, kCFAllocatorNull ) );
	cfauto<CFMutableStringRef> ms( CFStringCreateMutableCopy( 0, 0, cfs ) );
	CFStringUppercase( ms, nullptr );
	return to_string( ms );
#elif UPLATFORM_WIN
	auto w = to_wstring(s);
	CharUpperW( w.data() );
	return to_string( w );
#else
	auto us = icu::UnicodeString::fromUTF8( icu::StringPiece( s.data(), s.size() ) );
	std::string s8;
	return us.toUpper().toUTF8String(s8);
#endif
}

int compare_nocase( const std::string_view &lhs, const std::string_view &rhs )
{
#if UPLATFORM_MAC || UPLATFORM_IOS
	cfauto<CFStringRef> lhsRef( CFStringCreateWithBytesNoCopy( 0, (const UInt8 *)lhs.data(), lhs.length(), kCFStringEncodingUTF8, false, kCFAllocatorNull ) );
	cfauto<CFStringRef> rhsRef( CFStringCreateWithBytesNoCopy( 0, (const UInt8 *)rhs.data(), rhs.length(), kCFStringEncodingUTF8, false, kCFAllocatorNull ) );
	return CFStringCompare( lhsRef, rhsRef, kCFCompareCaseInsensitive+kCFCompareNonliteral );
#elif UPLATFORM_WIN
	return tolower(lhs).compare(tolower(rhs));
#else
	auto lhsus = icu::UnicodeString::fromUTF8( icu::StringPiece( lhs.data(), lhs.size() ) );
	auto rhsus = icu::UnicodeString::fromUTF8( icu::StringPiece( rhs.data(), rhs.size() ) );
	return lhsus.caseCompare( rhsus, U_FOLD_CASE_DEFAULT );
#endif
}

int ucompare( const std::string_view &lhs, const std::string_view &rhs )
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

int ucompare_nocase( const std::string_view &lhs, const std::string_view &rhs )
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

int ucompare_numerically( const std::string_view &lhs, const std::string_view &rhs )
{
	size_t la = lhs.length(), lb = rhs.length();
	size_t shortest = std::min( la, lb );
	const char *pa = lhs.data();
	const char *pb = rhs.data();
	size_t i = 0;
	
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

int ucompare_nocase_numerically( const std::string_view &lhs, const std::string_view &rhs )
{
	size_t la = lhs.length(), lb = rhs.length();
	size_t shortest = std::min( la, lb );
	const char *pa = lhs.data();
	const char *pb = rhs.data();
	size_t i = 0;
	
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

std::string_view trim_spaces_view( const std::string_view &i_s )
{
	auto f = i_s.find_first_not_of( "\r\n\t " );
	auto l = i_s.find_last_not_of( "\r\n\t " );
	if ( f != std::string_view::npos )
	{
		assert( l != std::string_view::npos );
		return i_s.substr( f, l - f + 1 );
	}
	else
        return {};
}

bool contains_nocase( const std::string_view &i_text, const std::string_view &i_s )
{
	return contains( tolower(i_text), tolower(i_s) );
}
bool starts_with( const std::string_view &i_text, const std::string_view &i_s )
{
	if ( i_s.size() > i_text.size() )
		return false;
    return i_text.substr( 0, i_s.size() ) == i_s;
}
bool starts_with_nocase( const std::string_view &i_text, const std::string_view &i_s )
{
	if ( i_s.size() > i_text.size() )
		return false;
    return compare_nocase( i_text.substr( 0, i_s.size() ), i_s ) == 0;
}
bool ends_with( const std::string_view &i_text, const std::string_view &i_s )
{
	if ( i_s.size() > i_text.size() )
		return false;
    return i_text.substr( i_text.size() - i_s.size(), i_s.size() ) == i_s;
}
bool ends_with_nocase( const std::string_view &i_text, const std::string_view &i_s )
{
	if ( i_s.size() > i_text.size() )
		return false;
    return compare_nocase( i_text.substr( i_text.size() - i_s.size(), i_s.size() ), i_s ) == 0;
}

std::string japanese_hiASCII_fix( const std::string_view &i_s )
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

namespace {

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

namespace su {

std::string kanjiNumberFix( const std::string_view &i_s )
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

size_t levenshtein_distance( const std::string_view &i_string,
							const std::string_view &i_target )
{
	auto string16 = to_u16string( i_string );
	auto target16 = to_u16string( i_target );

	using std::u16string_view;

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
