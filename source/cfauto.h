//
//  cfauto.h
//  sutils
//
//  Created by Sandy Martel on 12/03/10.
//  Copyright (c) 2015年 Sandy Martel. All rights reserved.
//
// Permission to use, copy, modify, distribute, and sell this software for any purpose is hereby
// granted without fee. The sotware is provided "AS-IS" and without warranty of any kind, express,
// implied or otherwise.

#ifndef H_SU_CFAUTO
#define H_SU_CFAUTO

#include "su_platform.h"

#if UPLATFORM_MAC || UPLATFORM_IOS

#include <CoreFoundation/CoreFoundation.h>

namespace su {

template <class T>
class cfauto final
{
public:
	cfauto() = default;
	explicit cfauto( T i_obj ) : obj( i_obj ) {}
	cfauto( const cfauto<T> &i_other ) : obj( i_other.obj )
	{
		if ( obj != nullptr )
			CFRetain( obj );
	}
	cfauto( const cfauto<T> &&i_other ) noexcept : obj( i_other.obj ) { i_other.obj = nullptr; }
	~cfauto()
	{
		if ( obj != nullptr )
			CFRelease( obj );
	}
	cfauto<T> &operator=( const cfauto<T> &i_other )
	{
		if ( this != &i_other and obj != i_other.obj )
		{
			if ( obj != nullptr )
				CFRelease( obj );
			obj = i_other.obj;
			if ( obj != nullptr )
				CFRetain( obj );
		}
		return *this;
	}
	cfauto<T> &operator=( const cfauto<T> &&i_other ) noexcept
	{
		if ( this != &i_other and obj != i_other.obj )
		{
			if ( obj != nullptr )
				CFRelease( obj );
			obj = i_other.obj;
			i_other.obj = nullptr;
		}
		return *this;
	}

	void reset( T i_obj ) // will take ownership
	{
		if ( i_obj != obj )
		{
			if ( obj != nullptr )
				CFRelease( obj );
			obj = i_obj;
		}
	}

	operator bool() const { return obj != nullptr; }
	operator T() const { return obj; }
	T *addr() { return &obj; }
	
	T get() const { return obj; }
	bool isNull() const { return obj == nullptr; }
	bool isOfType( CFTypeID i_type ) const { return not isNull() and CFGetTypeID( obj ) == i_type; }
	bool isArray() const { return isOfType( CFArrayGetTypeID() ); }
	bool isDictionary() const { return isOfType( CFDictionaryGetTypeID() ); }
	bool isString() const { return isOfType( CFStringGetTypeID() ); }
	bool isNumber() const { return isOfType( CFNumberGetTypeID() ); }
	bool isBoolean() const { return isOfType( CFBooleanGetTypeID() ); }
	bool isURL() const { return isOfType( CFURLGetTypeID() ); }
	bool isUUID() const { return isOfType( CFUUIDGetTypeID() ); }
private:
	T obj = nullptr;
};

}

#endif

#endif
