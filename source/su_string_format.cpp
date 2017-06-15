/*
 *  su_string_format.cpp
 *  sutils
 *
 *  Created by Sandy Martel on 7/10/05.
 *  Copyright (c) 2015年 Sandy Martel. All rights reserved.
 *
 * Permission to use, copy, modify, distribute, and sell this software for any purpose is hereby
 * granted without fee. The sotware is provided "AS-IS" and without warranty of any kind, express,
 * implied or otherwise.
 */

#include "su_string_format.h"
#include "su_stackarray.h"
#include <vector>
#include <cassert>
#include <algorithm>
#include <cctype>

namespace su {

std::string format( const su::string_view &i_format )
{
	std::string res;
	if ( not i_format.empty() )
	{
		res.reserve( i_format.length() );
		auto ptr = i_format.begin();
		while ( ptr != i_format.end() )
		{
			if ( *ptr == '%' )
			{
				++ptr;
				if ( ptr != i_format.end() )
					res.push_back( *ptr++ );
			}
			else
				res.push_back( *ptr++ );
		}
	}
	return res;
}

namespace details {

struct FormatSpec
{
	int valueIndex = -1;
	int width = -1, prec = -1;
	int widthIndex = -1, precIndex = -1;
	ptrdiff_t size = 0;
	uint16_t flags = 0;
	char type = 0; // d, i, x, s, f, c, etc
};

FormatArg::~FormatArg()
{
	if ( _which == arg_type::kOther )
		delete u._other;
}

template<>
inline double FormatArg::cast() const
{
	switch ( _which )
	{
		case arg_type::kShort: return u._short;
		case arg_type::kInt: return u._int;
		case arg_type::kLong: return u._long;
		case arg_type::kLongLong: return static_cast<double>(u._longlong);
		case arg_type::kUnsignedShort: return u._unsignedshort;
		case arg_type::kUnsignedInt: return u._unsignedint;
		case arg_type::kUnsignedLong: return u._unsignedlong;
		case arg_type::kUnsignedLongLong: return static_cast<double>(u._unsignedlonglong);
		case arg_type::kDouble: return u._double;
		default:
			break;
	}
	throw std::runtime_error( "wrong argument type" );
}

template<>
inline int FormatArg::cast() const
{
	switch ( _which )
	{
		case arg_type::kShort: return u._short;
		case arg_type::kInt: return u._int;
		case arg_type::kUnsignedShort: return u._unsignedshort;
		case arg_type::kUnsignedInt: return static_cast<int>(u._unsignedint);
		default:
			break;
	}
	throw std::runtime_error( "wrong argument type" );
}

template<>
inline unsigned int FormatArg::cast() const
{
	switch ( _which )
	{
		case arg_type::kShort: return static_cast<unsigned int>(u._short);
		case arg_type::kInt: return static_cast<unsigned int>(u._int);
		case arg_type::kUnsignedShort: return u._unsignedshort;
		case arg_type::kUnsignedInt: return u._unsignedint;
		default:
			break;
	}
	throw std::runtime_error( "wrong argument type" );
}

template<>
inline long FormatArg::cast() const
{
	switch ( _which )
	{
		case arg_type::kShort: return u._short;
		case arg_type::kInt: return u._int;
		case arg_type::kUnsignedShort: return u._unsignedshort;
		case arg_type::kUnsignedInt: return u._unsignedint;
		case arg_type::kLong: return u._long;
		case arg_type::kUnsignedLong: return u._unsignedlong;
		default:
			break;
	}
	throw std::runtime_error( "wrong argument type" );
}

template<>
inline unsigned long FormatArg::cast() const
{
	switch ( _which )
	{
		case arg_type::kShort: return u._short;
		case arg_type::kInt: return u._int;
		case arg_type::kUnsignedShort: return u._unsignedshort;
		case arg_type::kUnsignedInt: return u._unsignedint;
		case arg_type::kLong: return u._long;
		case arg_type::kUnsignedLong: return u._unsignedlong;
		default:
			break;
	}
	throw std::runtime_error( "wrong argument type" );
}

template<>
inline short FormatArg::cast() const
{
	switch ( _which )
	{
		case arg_type::kShort: return u._short;
		case arg_type::kUnsignedShort: return u._unsignedshort;
		case arg_type::kInt: return static_cast<short>(u._int);
		case arg_type::kUnsignedInt: return static_cast<short>(u._unsignedint);
		default:
			break;
	}
	throw std::runtime_error( "wrong argument type" );
}

template<>
inline unsigned short FormatArg::cast() const
{
	switch ( _which )
	{
		case arg_type::kShort: return u._short;
		case arg_type::kUnsignedShort: return u._unsignedshort;
		case arg_type::kInt: return static_cast<unsigned short>(u._int);
		case arg_type::kUnsignedInt: return static_cast<unsigned short>(u._unsignedint);
		default:
			break;
	}
	throw std::runtime_error( "wrong argument type" );
}

template<>
inline long long FormatArg::cast() const
{
	switch ( _which )
	{
		case arg_type::kShort: return u._short;
		case arg_type::kInt: return u._int;
		case arg_type::kLong: return u._long;
		case arg_type::kLongLong: return u._longlong;
		case arg_type::kUnsignedShort: return u._unsignedshort;
		case arg_type::kUnsignedInt: return u._unsignedint;
		case arg_type::kUnsignedLong: return u._unsignedlong;
		case arg_type::kUnsignedLongLong: return u._unsignedlonglong;
		default:
			break;
	}
	throw std::runtime_error( "wrong argument type" );
}

template<>
inline unsigned long long FormatArg::cast() const
{
	switch ( _which )
	{
		case arg_type::kShort: return u._short;
		case arg_type::kInt: return u._int;
		case arg_type::kLong: return u._long;
		case arg_type::kLongLong: return u._longlong;
		case arg_type::kUnsignedShort: return u._unsignedshort;
		case arg_type::kUnsignedInt: return u._unsignedint;
		case arg_type::kUnsignedLong: return u._unsignedlong;
		case arg_type::kUnsignedLongLong: return u._unsignedlonglong;
		default:
			break;
	}
	throw std::runtime_error( "wrong argument type" );
}

std::string FormatArg::to_string() const
{
	switch ( _which )
	{
		case arg_type::kShort: return std::to_string( u._short );
		case arg_type::kInt: return std::to_string( u._int );
		case arg_type::kLong: return std::to_string( u._long );
		case arg_type::kLongLong: return std::to_string( u._longlong );
		case arg_type::kUnsignedShort: return std::to_string( u._unsignedshort );
		case arg_type::kUnsignedInt: return std::to_string( u._unsignedint );
		case arg_type::kUnsignedLong: return std::to_string( u._unsignedlong );
		case arg_type::kUnsignedLongLong: return std::to_string( u._unsignedlonglong );
		case arg_type::kSize: return std::to_string( u._size_t );
		case arg_type::kPtrDiff: return std::to_string( u._ptrdiff_t );
		case arg_type::kDouble: return std::to_string( u._double );
		case arg_type::kChar: return std::string( 1, u._char );
		case arg_type::kPtr: return std::string( "xx" );
		case arg_type::kString: return _string;
		case arg_type::kOther: return u._other->to_string();
		default:
			break;
	}
	throw std::runtime_error( "wrong argument type" );
}

} }

namespace {

enum : uint16_t
{
	// flags
	kLeftAdjustFlag = 0x01, // -
	kPlusFlag = 0x02,
	kSharpFlag = 0x04,
	kSpaceFlag = 0x08,
	kZeroFlag = 0x10,
	
	// lengths
	kShortLength = 0x0100,
	kLongLength = 0x0200,
	kLongLongLength = 0x0300,
	kSizeLength = 0x0400,
	kPtrDiffLength = 0x0500,
};

su::details::FormatSpec parseFormat( const su::string_view &i_ptr )
{
	//%[index$][flags][width][.precision][length]specifier
	
	su::details::FormatSpec formatSpec;
	
	auto ptr = i_ptr.begin();
	
	enum stage_t
	{
		kParsingIndex, kParsingFlags, kParsingWidth, kParsingPrecision, kParsingLength, kParsingSpecifier, kParserDone, kParserFailed
	};
	stage_t stage = kParsingIndex;
	int v;
	while ( ptr < i_ptr.end() and stage < kParserDone )
	{
		switch ( stage )
		{
			case kParsingIndex:
			{
				// \d+$ index for re-ordering
				stage = kParsingFlags; // next stage
				if ( *ptr > '0' and *ptr <= '9' ) // start with non-zero digit
				{
					// index (or might be width ?)
					v = 0;
					while ( ptr < i_ptr.end() and std::isdigit( *ptr ) )
					{
						v = (v * 10) + (*ptr - '0');
						++ptr;
					}
					if ( ptr < i_ptr.end() and *ptr == '$' )
					{
						// this is really an index
						++ptr; // skip $
						formatSpec.valueIndex = v - 1;
					}
					else
					{
						// this is the width
						formatSpec.width = v;
						stage = kParsingPrecision; // go straight to precision
						break;
					}
				}
				// fallthrough...
			}
			case kParsingFlags:
			{
				// -		left adjust
				// +		+/- for signed value
				// space	if no signed is to be printed, print a space
				// #		float always with decimal point, octal always with 0 hex always with 0x
				// 0		pad numbers with 0
				while ( ptr < i_ptr.end() and stage == kParsingFlags )
				{
					switch ( *ptr )
					{
						case '-':
							++ptr;
							formatSpec.flags |= kLeftAdjustFlag;
							break;
						case '+':
							++ptr;
							formatSpec.flags |= kPlusFlag;
							break;
						case ' ':
							++ptr;
							formatSpec.flags |= kSpaceFlag;
							break;
						case '#':
							++ptr;
							formatSpec.flags |= kSharpFlag;
							break;
						case '0':
							++ptr;
							formatSpec.flags |= kZeroFlag;
							break;
						default:
							stage = kParsingWidth;
							break;
					}
				}
				// fallthrough...
			}
			case kParsingWidth:
			{
				// \d+ or * field width, * mean get it from the args
				if ( ptr < i_ptr.end() )
				{
					if ( *ptr > '0' and *ptr <= '9' )
					{
						v = 0;
						while ( ptr < i_ptr.end() and std::isdigit( *ptr ) )
						{
							v = (v * 10) + (*ptr - '0');
							++ptr;
						}
						formatSpec.width = v;
					}
					else if ( *ptr == '*' )
					{
						++ptr; // skip *
						formatSpec.widthIndex = -2; // flag it for later
					}
				}
				stage = kParsingPrecision;
				// fallthrough...
			}
			case kParsingPrecision:
			{
				// \d+ or * precision, * mean get it from the args, default to 6
				if ( ptr < i_ptr.end() and *ptr == '.' )
				{
					++ptr;
					if ( ptr < i_ptr.end() )
					{
						if ( std::isdigit( *ptr ) )
						{
							v = 0;
							while ( ptr < i_ptr.end() and std::isdigit( *ptr ) )
							{
								v = (v * 10) + (*ptr - '0');
								++ptr;
							}
							formatSpec.prec = v;
						}
						else if ( *ptr == '*' )
						{
							++ptr; // skip *
							formatSpec.precIndex = -2; // flag it for later
						}
					}
				}
				stage = kParsingLength;
				// fallthrough...
			}
			case kParsingLength:
			{
				if ( ptr < i_ptr.end() )
				{
					// h	next d, o, x or u is a short
					// l	next d, o, x or u is a long
					// ll	next d, o, x or u is a long long
					// z	size_t
					// t	ptrdiff_t
					switch ( *ptr )
					{
						case 'h':
							++ptr;
							formatSpec.flags |= kShortLength;
							break;
						case 'l':
							++ptr;
							if ( ptr < i_ptr.end() and *ptr == 'l' ) // for ll
							{
								++ptr;
								formatSpec.flags |= kLongLongLength;
							}
							else
							{
								formatSpec.flags |= kLongLength;
							}
							break;
						case 'z':
							++ptr;
							formatSpec.flags |= kSizeLength;
							break;
						case 't':
							++ptr;
							formatSpec.flags |= kPtrDiffLength;
							break;
						default:
							break;
					}
				}
				stage = kParsingSpecifier;
				// fallthrough...
			}
			case kParsingSpecifier:
			{
				if ( ptr < i_ptr.end() )
				{
					// d or i	integer decimal
					// u		unsigned integer decimal
					// o		unsigned integer octal
					// x		unsigned integer hex 0x
					// X		unsigned integer hex 0X
					// b		unsigned integer binary 0b
					// B		unsigned integer binary 0B
					// f or F	float [-]ddd.ddd
					// e		float [-]d.ddde+dd or [-]d.ddde-dd
					// E		float [-]d.dddE+dd or [-]d.dddE-dd
					// g		d, f or e, whichever gives greatest precision
					// G		d, F or E, whichever gives greatest precision
					// c		character
					// s		string
					// p		pointer
					// @		any thing convertible to a string (std::to_string)
					switch ( *ptr )
					{
						case 'd': case 'i':
						case 'B': case 'b':
						case 'u': case 'o': case 'x': case 'X':
						case 'f': case 'F': case 'e': case 'E':
						case 'g': case 'G':
						case 'a': case 'A':
						case 'c': case 's': case 'p':
						case '@': case 'n':
							formatSpec.type = *ptr;
							++ptr;
							stage = kParserDone;
							formatSpec.size = ptr - i_ptr.begin();
							return formatSpec;
						default:
							// if we already have a length, set the type to int
							if ( formatSpec.flags&0x0F00 )
							{
								formatSpec.type = 'd';
								stage = kParserDone;
								formatSpec.size = ptr - i_ptr.begin();
								return formatSpec;
							}
							stage = kParserFailed;
							throw std::runtime_error( "invalid format string" );
					}
				}
				break;
			}
			default:
				break;
		}
	}
	return formatSpec;
}

template<typename INTEGER>
int count_dec_digits( INTEGER n )
{
	int count = 1;
	for (;;)
	{
		// Integer division is slow so do it for a group of four digits instead
		// of for every digit. The idea comes from the talk by Alexandrescu
		// "Three Optimization Tips for C++". See speed-test for a comparison.
		if ( n < 10 )
			return count;
		if ( n < 100 )
			return count + 1;
		if ( n < 1000 )
			return count + 2;
		if ( n < 10000 )
			return count + 3;
		n /= 10000u;
		count += 4;
	}
	return count;
}
template<typename INTEGER>
int count_hex_digits( INTEGER n )
{
	int count = 0;
	do
	{
		++count;
	} while ((n >>= 4) != 0);
	return count;
}
template<typename INTEGER>
int count_oct_digits( INTEGER n )
{
	int count = 0;
	do
	{
		++count;
	} while ((n >>= 3) != 0);
	return count;
}
template<typename INTEGER>
int count_bin_digits( INTEGER n )
{
	int count = 0;
	do
	{
		++count;
	} while ((n >>= 1) != 0);
	return count;
}

void appendIntegerSimple( char *&io_ptr, int i_arg )
{
	unsigned int abs_value = i_arg;
	if ( i_arg < 0 )
	{
		*io_ptr++ = '-';
		abs_value = 0 - i_arg;
	}
	auto num_digits = count_dec_digits( abs_value );
	io_ptr += num_digits;
	auto ptr = io_ptr - 1;
	do
	{
		*ptr-- = (abs_value%10) + '0';
	}
	while ( (abs_value /= 10) != 0 );
}

}

namespace su { namespace details {

format_impl::format_impl( const su::string_view &i_format, const FormatArg *i_args, size_t i_argsSize )
{
	std::vector<su::details::FormatSpec> formatSpecs;
	formatSpecs.reserve( i_argsSize );
	
	// loop to extract the format spec for each placeholder in i_format.
	auto ptr = i_format.begin();
	while ( ptr < i_format.end() )
	{
		if ( *ptr == '%' )
		{
			++ptr;
			if ( ptr < i_format.end() and *ptr != '%' )
			{
				formatSpecs.push_back( parseFormat( ptr ) );
				ptr += formatSpecs.back().size;
				continue;
			}
		}
		++ptr;
	}
	
	// fix up the indexes
	if ( not formatSpecs.empty() )
	{
		int argIndex = 0;
		if ( formatSpecs[0].valueIndex != -1 )
		{
			// re-ordered, the same arg can be re-used more than once
			std::vector<size_t> toVisit;
			toVisit.reserve( formatSpecs.size() );
			for ( const auto &it : formatSpecs )
			{
				if ( it.valueIndex == -1 )
					throw std::runtime_error( "cannot mix indexed and non-indexed arguments" );
				toVisit.push_back( toVisit.size() );
			}
			
			int visiting = 0;
			while ( not toVisit.empty() )
			{
				// find if any width or prec index is needed
				bool needWidthIndex = false, needPrecIndex = false, found = false;
				for ( auto i : toVisit )
				{
					if ( formatSpecs[i].valueIndex == visiting )
					{
						if ( formatSpecs[i].widthIndex == -2 )
							needWidthIndex = true;
						if ( formatSpecs[i].precIndex == -2 )
							needPrecIndex = true;
						found = true;
					}
				}
				if ( found )
				{
					int widthIndex = needWidthIndex ? argIndex++ : -1;
					int precIndex = needPrecIndex ? argIndex++ : -1;
					int index = argIndex++;
					for ( auto i : toVisit )
					{
						if ( formatSpecs[i].valueIndex == visiting )
						{
							formatSpecs[i].widthIndex = widthIndex;
							formatSpecs[i].precIndex = precIndex;
							formatSpecs[i].valueIndex = index;
						}
					}
					toVisit.erase( std::remove_if( toVisit.begin(), toVisit.end(), [&]( size_t v ){ return formatSpecs[v].valueIndex == visiting; } ), toVisit.end() );
				}
				++visiting;
			}
		}
		else
		{
			// straight order
			for ( auto &it : formatSpecs )
			{
				if ( it.widthIndex == -2 )
					it.widthIndex = argIndex++;
				if ( it.precIndex == -2 )
					it.precIndex = argIndex++;
				if ( it.valueIndex != -1 )
					throw std::runtime_error( "cannot mix indexed and non-indexed arguments" );
				it.valueIndex = argIndex++;
			}
		}
	}
	
	result.reserve( i_format.size() ); // should be at least that size
	
	// loop to build the final string
	ptr = i_format.begin();
	auto current = ptr;
	int formatSpecIndex = 0;
	while ( ptr < i_format.end() )
	{
		if ( *ptr == '%' )
		{
			result.append( current, ptr );
			++ptr;
			if ( ptr < i_format.end() and *ptr != '%' )
			{
				appendFormattedArg( formatSpecs[formatSpecIndex], i_args, i_argsSize );
				ptr += formatSpecs[formatSpecIndex].size;
				++formatSpecIndex;
			}
			current = ptr;
			if ( formatSpecIndex >= (int)formatSpecs.size() )
				break; // no more args, give up
		}
		else
			++ptr;
	}
	result.append( current, i_format.end() );
}

void format_impl::appendFormattedArg( const su::details::FormatSpec &i_formatSpec, const su::details::FormatArg *i_args, size_t i_argsSize )
{
	if ( i_formatSpec.valueIndex < 0 )
		throw std::runtime_error( "missing argument" );
	if ( i_formatSpec.valueIndex >= (int)i_argsSize )
		throw std::out_of_range( "index out of range" );
	
	const su::details::FormatArg &arg = i_args[i_formatSpec.valueIndex];
	
	int width;
	if ( i_formatSpec.widthIndex < 0 )
		width = i_formatSpec.width;
	else if ( i_formatSpec.widthIndex < (int)i_argsSize )
		width = i_args[i_formatSpec.widthIndex].cast<int>();
	else
		throw std::out_of_range( "index out of range" );
	
	int prec;
	if ( i_formatSpec.precIndex < 0 )
		prec = i_formatSpec.prec;
	else if ( i_formatSpec.precIndex < (int)i_argsSize )
		prec = i_args[i_formatSpec.precIndex].cast<int>();
	else
		throw std::out_of_range( "index out of range" );
	
	switch ( i_formatSpec.type )
	{
		case 'd': case 'i':
		{
			switch ( i_formatSpec.flags&0xFF00 )
			{
				case kShortLength:
					formatInteger( arg.cast<short>(), i_formatSpec, width, prec );
					break;
				case kLongLength:
					formatInteger( arg.cast<long>(), i_formatSpec, width, prec );
					break;
				case kLongLongLength:
					formatInteger( arg.cast<long long>(), i_formatSpec, width, prec );
					break;
				case kSizeLength:
					formatInteger( arg.cast<size_t>(), i_formatSpec, width, prec );
					break;
				case kPtrDiffLength:
					formatInteger( arg.cast<ptrdiff_t>(), i_formatSpec, width, prec );
					break;
				default:
					formatInteger( arg.cast<int>(), i_formatSpec, width, prec );
					break;
			}
			break;
		}
		case 'u': case 'o':
		case 'x': case 'X':
		case 'b': case 'B':
		{
			switch ( i_formatSpec.flags&0xFF00 )
			{
				case kShortLength:
					formatInteger( arg.cast<unsigned short>(), i_formatSpec, width, prec );
					break;
				case kLongLength:
					formatInteger( arg.cast<unsigned long>(), i_formatSpec, width, prec );
					break;
				case kLongLongLength:
					formatInteger( arg.cast<unsigned long long>(), i_formatSpec, width, prec );
					break;
				case kSizeLength: case kPtrDiffLength:
					formatInteger( arg.cast<size_t>(), i_formatSpec, width, prec );
					break;
				default:
					formatInteger( arg.cast<unsigned int>(), i_formatSpec, width, prec );
					break;
			}
			break;
		}
		case 'f': case 'F':
		case 'e': case 'E':
		case 'g': case 'G':
		case 'a': case 'A':
			formatDouble( arg.cast<double>(), i_formatSpec, width, prec );
			break;
		case 'p':
		case 'n':
			assert( false );
			break;
		case 'c':
		case 's':
		case '@':
		{
			// only width, prec and kLeftAdjustFlag are relevant
			std::string s;
			s = arg.to_string();
			if ( prec == -1 )
				prec = (int)s.size();
			else
				prec = std::min( prec, (int)s.size() );
			if ( width < prec )
				width = prec;
			
			if ( i_formatSpec.flags&kLeftAdjustFlag  )
			{
				result.append( s, 0, prec );
				result.append( width - prec, ' ' );
			}
			else
			{
				result.append( width - prec, i_formatSpec.flags&kZeroFlag ? '0' : ' ' );
				result.append( s, 0, prec );
			}
			break;
		}
		default:
			assert( false );
			break;
	}
}

char *format_impl::prepare_append_integer( int num_digits, uint16_t flags, int width, int prec, const char *prefix, int prefix_size )
{
	if ( prec < num_digits )
		prec = num_digits;
	auto len = prec + prefix_size;
	if ( width < len )
		width = len;
	
	if ( flags&kLeftAdjustFlag )
	{
		result.append( prefix, prefix_size );
		result.append( prec, '0' );
		auto padding = width - prefix_size - prec;
		result.append(padding, ' ' );
		return const_cast<char *>(result.data()) + result.size() - 1 - padding;
	}
	else if ( flags&kZeroFlag )
	{
		result.append( prefix, prefix_size );
		result.append( width - prefix_size, '0' );
	}
	else
	{
		result.append( width - prec - prefix_size, ' ' );
		result.append( prefix, prefix_size );
		result.append( prec, '0' );
	}
	
	return const_cast<char *>(result.data()) + result.size() - 1;
}

template<typename T>
void format_impl::formatInteger( T i_arg, const su::details::FormatSpec &i_formatSpec, int width, int prec )
{
	int prefix_size = 0;
	char prefix[4] = "";
	typename std::make_unsigned_t<T> abs_value = i_arg;
	if ( i_arg < 0 )
	{
		prefix[0] = '-';
		++prefix_size;
		abs_value = 0 - i_arg;
	}
	else if ( i_formatSpec.flags&kPlusFlag )
	{
		prefix[0] = '+';
		++prefix_size;
	}
	else if ( i_formatSpec.flags&kSpaceFlag )
	{
		prefix[0] = ' ';
		++prefix_size;
	}
	switch ( i_formatSpec.type )
	{
		case 'd': case 'i': case 'u':
		{
			auto num_digits = count_dec_digits( abs_value );
			auto ptr = prepare_append_integer( num_digits, i_formatSpec.flags, width, prec, prefix, prefix_size );
			do
			{
				*ptr-- = (abs_value%10) + '0';
			}
			while ( (abs_value /= 10) != 0 );
			break;
		}
		case 'x': case 'X':
		{
			if ( i_formatSpec.flags&kSharpFlag )
			{
				prefix[prefix_size++] = '0';
				prefix[prefix_size++] = i_formatSpec.type;
			}
			auto num_digits = count_hex_digits( abs_value );
			auto ptr = prepare_append_integer( num_digits, i_formatSpec.flags, width, prec, prefix, prefix_size );
			const char *digits = (i_formatSpec.type == 'x') ? "0123456789abcdef" : "0123456789ABCDEF";
			do
			{
				*ptr-- = digits[abs_value&0x0F];
			}
			while ( (abs_value >>= 4) != 0 );
			break;
		}
		case 'b': case 'B':
		{
			if ( i_formatSpec.flags&kSharpFlag )
			{
				prefix[prefix_size++] = '0';
				prefix[prefix_size++] = i_formatSpec.type;
			}
			auto num_digits = count_bin_digits( abs_value );
			auto ptr = prepare_append_integer( num_digits, i_formatSpec.flags, width, prec, prefix, prefix_size );
			do
			{
				*ptr-- = '0' + (abs_value&1);
			}
			while ( (abs_value >>= 1) != 0 );
			break;
		}
		case 'o':
		{
			if ( i_formatSpec.flags&kSharpFlag )
				prefix[prefix_size++] = '0';
			auto num_digits = count_oct_digits( abs_value );
			auto ptr = prepare_append_integer( num_digits, i_formatSpec.flags, width, prec, prefix, prefix_size );
			do
			{
				*ptr-- = '0' + (abs_value & 7);
			}
			while ( (abs_value >>= 3) != 0 );
			break;
		}
		default:
			break;
	}
}

void format_impl::formatDouble( double i_arg, const su::details::FormatSpec &i_formatSpec, int width, int prec )
{
	char format[32];
	char *ptr = format;
	*ptr++ = '%';
	if ( i_formatSpec.flags&kLeftAdjustFlag )
		*ptr++ = '-';
	if ( i_formatSpec.flags&kPlusFlag )
		*ptr++ = '+';
	if ( i_formatSpec.flags&kSharpFlag )
		*ptr++ = '#';
	if ( i_formatSpec.flags&kSpaceFlag )
		*ptr++ = ' ';
	if ( i_formatSpec.flags&kZeroFlag )
		*ptr++ = '0';
	
	if ( width == -1 )
	{
		if ( prec != -1 )
		{
			*ptr++ = '.';
			appendIntegerSimple( ptr, prec );
		}
	}
	else if ( prec == -1 )
	{
		appendIntegerSimple( ptr, width );
	}
	else
	{
		appendIntegerSimple( ptr, width );
		*ptr++ = '.';
		appendIntegerSimple( ptr, prec );
	}
	*ptr++ = i_formatSpec.type;
	*ptr++ = 0;
	
	stackarray<char,64> buffer;
	for ( ;; )
	{
		auto req = snprintf( buffer, buffer.size(), format, i_arg );
		if ( req < (int)buffer.size() )
			break;
		buffer.realloc( std::max<size_t>( req+1, buffer.size() * 2 ) );
	}
	
	result.append( buffer );
}

} }
