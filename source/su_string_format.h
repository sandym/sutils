/*
 *  su_string_format.h
 *  sutils
 *
 *  Created by Sandy Martel on 7/10/05.
 *  Copyright (c) 2015年 Sandy Martel. All rights reserved.
 *
 * Permission to use, copy, modify, distribute, and sell this software for any purpose is hereby
 * granted without fee. The sotware is provided "AS-IS" and without warranty of any kind, express,
 * implied or otherwise.
 */
/*!
 	@header su_string.h
	Define a multiplatform unicode string class and some localisation utilities.
*/

#ifndef H_SU_STRING_FORMAT
#define H_SU_STRING_FORMAT

#include <ciso646>
#include <string>
#include "shim/string_view.h"
#include "su_always_inline.h"

namespace su { namespace details {

struct FormatSpec;
class FormatArg
{
public:
	FormatArg( short v ) : _which( arg_type::kShort ){ u._short = v; }
	FormatArg( int v ) : _which( arg_type::kInt ){ u._int = v; }
	FormatArg( long v ) : _which( arg_type::kLong ){ u._long = v; }
	FormatArg( long long v ) : _which( arg_type::kLongLong ){ u._longlong = v; }
	FormatArg( unsigned short v ) : _which( arg_type::kUnsignedShort ){ u._unsignedshort = v; }
	FormatArg( unsigned int v ) : _which( arg_type::kUnsignedInt ){ u._unsignedint = v; }
	FormatArg( unsigned long v ) : _which( arg_type::kUnsignedLong ){ u._unsignedlong = v; }
	FormatArg( unsigned long long v ) : _which( arg_type::kUnsignedLongLong ){ u._unsignedlonglong = v; }
	FormatArg( double v ) : _which( arg_type::kDouble ){ u._double = v; }
	FormatArg( float v ) : _which( arg_type::kDouble ){ u._double = v; }
	FormatArg( char v ) : _which( arg_type::kChar ){ u._char = v; }
	FormatArg( const void *v ) : _which( arg_type::kPtr ){ u._ptr = v; }
	FormatArg( void *v ) : _which( arg_type::kPtr ){ u._ptr = v; }
	FormatArg( const char *v ) : _which( arg_type::kString ), _string( v ){}
	FormatArg( char *v ) : _which( arg_type::kString ), _string( v ){}
	FormatArg( const std::string &v ) : _which( arg_type::kString ), _string( v ){}
	FormatArg( const su::string_view &v ) : _which( arg_type::kString ), _string( v ){}

	template<typename T,typename = std::enable_if_t<not std::is_base_of<std::string,T>::value>>
	FormatArg( const T &v ) : _which( arg_type::kOther ){ u._other = new any<T>( v ); }
	FormatArg( FormatArg &&i_other ) noexcept : _which( i_other._which )
	{
		u = i_other.u;
		i_other.u._other = nullptr;
	}
	FormatArg( const FormatArg & ) = delete;
	FormatArg &operator=( const FormatArg & ) = delete;
	~FormatArg();
	
	// to deal with POD types
	template<typename T>
	T cast() const;
	
	// to deal with string and others
	std::string to_string() const;
	
private:
	struct any_base
	{
		virtual ~any_base() = default;
		virtual std::string to_string() const = 0;
	};

	enum class arg_type : uint8_t
	{
		kShort, kInt, kLong, kLongLong,
		kUnsignedShort, kUnsignedInt, kUnsignedLong, kUnsignedLongLong,
		kSize, kPtrDiff,
		kDouble, kChar, kPtr, kString, kOther
	} _which;
	union
	{
		short _short;
		int _int;
		long _long;
		long long _longlong;
		unsigned short _unsignedshort;
		unsigned int _unsignedint;
		unsigned long _unsignedlong;
		unsigned long long _unsignedlonglong;
		size_t _size_t;
		std::ptrdiff_t _ptrdiff_t;
		double _double;
		char _char;
		const void *_ptr;
		any_base *_other;
	} u;
	su::string_view _string;

	template<typename T>
	struct any : public any_base
	{
		any( const T &i_value ) : value( i_value ){}
		virtual ~any() = default;
		virtual std::string to_string() const { using namespace std; return to_string( *value ); }
		const T &value;
	};
};

class format_impl
{
public:
	format_impl( const su::string_view &i_format, const su::details::FormatArg *i_args, size_t i_argsSize );

	std::string result;
	
private:
	void appendFormattedArg( const su::details::FormatSpec &i_formatSpec, const su::details::FormatArg *i_args, size_t i_argsSize );
	template<typename T>
	void formatInteger( T i_arg, const su::details::FormatSpec &i_formatSpec, int width, int prec );
	char *prepare_append_integer( int num_digits, uint16_t flags, int width, int prec, const char *prefix, int prefix_size );
	void formatDouble( double i_arg, const su::details::FormatSpec &i_formatSpec, int width, int prec );
};
}

/*!
   @brief  A printf-like formatter function.

		It accept all the printf format including i18n formatting (i.e. %1$s to reorder the arguments).
		It also accept %@ to print anything that can be converted to a string (with a to_string overload)

   @param[in] i_format a format string
   @return     formatted string
*/
std::string format( const su::string_view &i_format ); // overloaded fast path
template<typename ...Args>
std::string never_inline_func format( const su::string_view &i_format, Args... args )
{
	// sizeof...(Args) is an upper limit on the number of arguments to format
	su::details::FormatArg arr[sizeof...(Args)] = {args...};
	return su::details::format_impl( i_format, arr, sizeof...(Args) ).result;
}

}

#endif
