/*
 *  su_usersettings.cpp
 *  sutils
 *
 *  Created by Sandy Martel on 08-03-27.
 *  Copyright (c) 2015年 Sandy Martel. All rights reserved.
 *
 * Permission to use, copy, modify, distribute, and sell this software for any purpose is hereby
 * granted without fee. The sotware is provided "AS-IS" and without warranty of any kind, express,
 * implied or otherwise.
 */

#include "su_usersettings.h"
//#include "su_stackarray.h"
#include "su_filepath.h"
#include "su_string.h"

namespace
{
std::string g_company, g_application;

std::string getPlatformKey()
{
	assert( not g_company.empty() );
	assert( not g_application.empty() );
#if UPLATFORM_WIN
	return std::string("Software\\" + g_company + "\\" + g_application;
#elif UPLATFORM_MAC || UPLATFORM_IOS
	return std::string("com.") + g_company + "." + g_application;
#else
#error
#endif
}

}

namespace su
{

void usersettings::init( const su_std::string_view &i_company, const su_std::string_view &i_application )
{
	g_company = i_company;
	g_application = i_application;
}

#if UPLATFORM_MAC || UPLATFORM_IOS

usersettings::usersettings()
{
	assert( false );
}

usersettings::~usersettings()
{
	assert( false );
}

template<typename T>
T usersettings::read( const su_std::string_view &i_name, const T &i_default )
{
	assert( false );
	return {};
}

template<typename T>
void usersettings::write( const su_std::string_view &i_name, const T &i_value )
{
	assert( false );
}

#endif

#if 0
usersettings::usersettings()
{
#if UPLATFORM_WIN
	_prefsKey = 0;
	if ( ::RegOpenKey( HKEY_CURRENT_USER, getPlatformKey().c_str(), &_prefsKey ) != ERROR_SUCCESS )
		throw std::runtime_error( "cannot open registry" );
#else
#error
#endif
}

usersettings::~usersettings()
{
#if UPLATFORM_WIN
	::RegCloseKey( _prefsKey );
#else
#error
#endif
}

#if UPLATFORM_MAC || UPLATFORM_IOS

template<>
bool usersettings::CFPropertyListTo( CFPropertyListRef v, int &o_result ) const
{
	int intValue;
	if ( v != nullptr and CFGetTypeID( v ) == CFNumberGetTypeID() and CFNumberGetValue( (CFNumberRef)v, kCFNumberIntType, &intValue ) )
	{
		o_result = intValue;
		return true;
	}
	else
		return false;
}

template<>
CFPropertyListRef usersettings::CFPropertyListCreateFrom( const int &i_value ) const
{
	return CFNumberCreate( 0, kCFNumberIntType, &i_value );
}

template<>
bool usersettings::CFPropertyListTo( CFPropertyListRef v, float &o_result ) const
{
	float floatValue;
	if ( v != nullptr and CFGetTypeID( v ) == CFNumberGetTypeID() and CFNumberGetValue( (CFNumberRef)v, kCFNumberFloatType, &floatValue ) )
	{
		o_result = floatValue;
		return true;
	}
	else
		return false;
}

template<>
CFPropertyListRef usersettings::CFPropertyListCreateFrom( const float &i_value ) const
{
	return CFNumberCreate( 0, kCFNumberFloatType, &i_value );
}

template<>
CFPropertyListRef usersettings::CFPropertyListCreateFrom( const filepath &i_value ) const
{
	filepath::BookmarkData bm;
	if ( i_value.getBookmarkData( bm ) )
		return CFDataCreate( 0, (const UInt8 *)bm.data(), bm.size() );
	return nullptr;
}

#elif UPLATFORM_WIN

template<>
ustring usersettings::read( const ustring &i_name, const ustring &i_default )
{
	DWORD len = 0, type;
	if ( ::RegQueryValueEx( _prefsKey, i_name.c_str(), 0, &type, 0, &len ) == ERROR_SUCCESS and type == REG_BINARY )
	{
		stack_array<char> buffer( len + 1 );
		if ( ::RegQueryValueEx( _prefsKey, i_name.c_str(), 0, &type, (BYTE *)buffer.c_array(), &len ) == ERROR_SUCCESS )
		{
			buffer.c_array()[len] = 0;
			return ustring( buffer.data() );
		}
	}
	return i_default;
}

template<>
void usersettings::write( const ustring &i_name, const ustring &i_value )
{
	std::string s = i_value.utf8();
	::RegSetValueEx( _prefsKey, i_name.c_str(), 0, REG_BINARY, (const BYTE *)s.data(), s.length() );
}

template<>
bool usersettings::read( const ustring &i_name, const bool &i_default )
{
	DWORD len = 0, type;
	if ( ::RegQueryValueEx( _prefsKey, i_name.c_str(), 0, &type, 0, &len ) == ERROR_SUCCESS and type == REG_BINARY )
	{
		stack_array<char> buffer( len + 1 );
		if ( ::RegQueryValueEx( _prefsKey, i_name.c_str(), 0, &type, (BYTE *)buffer.c_array(), &len ) == ERROR_SUCCESS )
		{
			buffer.c_array()[len] = 0;
			return ustring( buffer.data() );
		}
	}
	return i_default;
}

template<>
void usersettings::write( const ustring &i_name, const bool &i_value )
{
	std::string s = i_value.utf8();
	::RegSetValueEx( _prefsKey, i_name.c_str(), 0, REG_BINARY, (const BYTE *)s.data(), s.length() );
}

template<>
int usersettings::read( const ustring &i_name, const int &i_default )
{
	DWORD len = 0, type;
	if ( ::RegQueryValueEx( _prefsKey, i_name.c_str(), 0, &type, 0, &len ) == ERROR_SUCCESS and type == REG_BINARY )
	{
		stack_array<char>	buffer( len + 1 );
		if ( ::RegQueryValueEx( _prefsKey, i_name.c_str(), 0, &type, (BYTE *)buffer.c_array(), &len ) == ERROR_SUCCESS )
		{
			buffer.c_array()[len] = 0;
			return ustring( buffer.data() );
		}
	}
	return i_default;
}

template<>
void usersettings::write( const ustring &i_name, const int &i_value )
{
	std::string s = i_value.utf8();
	::RegSetValueEx( _prefsKey, i_name.c_str(), 0, REG_BINARY, (const BYTE *)s.data(), s.length() );
}

template<>
float usersettings::read( const ustring &i_name, const float &i_default )
{
	DWORD len = 0, type;
	if ( ::RegQueryValueEx( _prefsKey, i_name.c_str(), 0, &type, 0, &len ) == ERROR_SUCCESS and type == REG_BINARY )
	{
		stack_array<char> buffer( len + 1 );
		if ( ::RegQueryValueEx( _prefsKey, i_name.c_str(), 0, &type, (BYTE *)buffer.c_array(), &len ) == ERROR_SUCCESS )
		{
			buffer.c_array()[len] = 0;
			return ustring( buffer.data() );
		}
	}
	return i_default;
}

template<>
void usersettings::write( const ustring &i_name, const float &i_value )
{
	std::string	s = i_value.utf8();
	::RegSetValueEx( _prefsKey, i_name.c_str(), 0, REG_BINARY, (const BYTE *)s.data(), s.length() );
}

template<>
double usersettings::read( const ustring &i_name, const double &i_default )
{
	DWORD len = 0, type;
	if ( ::RegQueryValueEx( _prefsKey, i_name.c_str(), 0, &type, 0, &len ) == ERROR_SUCCESS and type == REG_BINARY )
	{
		stack_array<char> buffer( len + 1 );
		if ( ::RegQueryValueEx( _prefsKey, i_name.c_str(), 0, &type, (BYTE *)buffer.c_array(), &len ) == ERROR_SUCCESS )
		{
			buffer.c_array()[len] = 0;
			return ustring( buffer.data() );
		}
	}
	return i_default;
}

template<>
void usersettings::write( const ustring &i_name, const double &i_value )
{
	std::string s = i_value.utf8();
	::RegSetValueEx( _prefsKey, i_name.c_str(), 0, REG_BINARY, (const BYTE *)s.data(), s.length() );
}

template<>
filepath usersettings::read( const ustring &i_name, const filepath &i_default )
{
	DWORD len = 0, type;
	if ( ::RegQueryValueEx( _prefsKey, i_name.c_str(), 0, &type, 0, &len ) == ERROR_SUCCESS and type == REG_BINARY )
	{
		stack_array<char> buffer( len + 1 );
		if ( ::RegQueryValueEx( _prefsKey, i_name.c_str(), 0, &type, (BYTE *)buffer.c_array(), &len ) == ERROR_SUCCESS )
		{
			buffer.c_array()[len] = 0;
			return ustring( buffer.data() );
		}
	}
	return i_default;
}

template<>
void usersettings::write( const ustring &i_name, const filepath &i_value )
{
	std::string s = i_value.utf8();
	::RegSetValueEx( _prefsKey, i_name.c_str(), 0, REG_BINARY, (const BYTE *)s.data(), s.length() );
}

#else
#error
#endif

#endif


template bool usersettings::read<bool>( const su_std::string_view &i_name, const bool &i_default );
template int usersettings::read<int>( const su_std::string_view &i_name, const int &i_default );
template long usersettings::read<long>( const su_std::string_view &i_name, const long &i_default );
template long long usersettings::read<long long>( const su_std::string_view &i_name, const long long &i_default );
template float usersettings::read<float>( const su_std::string_view &i_name, const float &i_default );
template double usersettings::read<double>( const su_std::string_view &i_name, const double &i_default );
template std::string usersettings::read<std::string>( const su_std::string_view &i_name, const std::string &i_default );
template filepath usersettings::read<filepath>( const su_std::string_view &i_name, const filepath &i_default );

template std::vector<bool> usersettings::read<std::vector<bool>>( const su_std::string_view &i_name, const std::vector<bool> &i_default );
template std::vector<int> usersettings::read<std::vector<int>>( const su_std::string_view &i_name, const std::vector<int> &i_default );
template std::vector<long> usersettings::read<std::vector<long>>( const su_std::string_view &i_name, const std::vector<long> &i_default );
template std::vector<long long> usersettings::read<std::vector<long long>>( const su_std::string_view &i_name, const std::vector<long long> &i_default );
template std::vector<float> usersettings::read<std::vector<float>>( const su_std::string_view &i_name, const std::vector<float> &i_default );
template std::vector<double> usersettings::read<std::vector<double>>( const su_std::string_view &i_name, const std::vector<double> &i_default );
template std::vector<std::string> usersettings::read<std::vector<std::string>>( const su_std::string_view &i_name, const std::vector<std::string> &i_default );
template std::vector<filepath> usersettings::read<std::vector<filepath>>( const su_std::string_view &i_name, const std::vector<filepath> &i_default );

template void usersettings::write<bool>( const su_std::string_view &i_name, const bool &i_value );
template void usersettings::write<int>( const su_std::string_view &i_name, const int &i_value );
template void usersettings::write<long>( const su_std::string_view &i_name, const long &i_value );
template void usersettings::write<long long>( const su_std::string_view &i_name, const long long &i_value );
template void usersettings::write<float>( const su_std::string_view &i_name, const float &i_value );
template void usersettings::write<double>( const su_std::string_view &i_name, const double &i_value );
template void usersettings::write<std::string>( const su_std::string_view &i_name, const std::string &i_value );
template void usersettings::write<filepath>( const su_std::string_view &i_name, const filepath &i_value );

template void usersettings::write<std::vector<bool>>( const su_std::string_view &i_name, const std::vector<bool> &i_value );
template void usersettings::write<std::vector<int>>( const su_std::string_view &i_name, const std::vector<int> &i_value );
template void usersettings::write<std::vector<long>>( const su_std::string_view &i_name, const std::vector<long> &i_value );
template void usersettings::write<std::vector<long long>>( const su_std::string_view &i_name, const std::vector<long long> &i_value );
template void usersettings::write<std::vector<float>>( const su_std::string_view &i_name, const std::vector<float> &i_value );
template void usersettings::write<std::vector<double>>( const su_std::string_view &i_name, const std::vector<double> &i_value );
template void usersettings::write<std::vector<std::string>>( const su_std::string_view &i_name, const std::vector<std::string> &i_value );
template void usersettings::write<std::vector<filepath>>( const su_std::string_view &i_name, const std::vector<filepath> &i_value );

void usersettings::write( const su_std::string_view &i_name, const char *i_value )
{
	write( i_name, std::string(i_value) );
}

}
