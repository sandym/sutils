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

	inline operator bool() const { return obj != nullptr; }
	inline operator T() const { return obj; }
	inline T *addr() { return &obj; }
	
	inline T get() const { return obj; }
	inline bool isNull() const { return obj == nullptr; }
	inline bool isOfType( CFTypeID i_type ) const { return not isNull() and CFGetTypeID( obj ) == i_type; }
	inline bool isArray() const { return isOfType( CFArrayGetTypeID() ); }
	inline bool isDictionary() const { return isOfType( CFDictionaryGetTypeID() ); }
	inline bool isString() const { return isOfType( CFStringGetTypeID() ); }
	inline bool isNumber() const { return isOfType( CFNumberGetTypeID() ); }
	inline bool isBoolean() const { return isOfType( CFBooleanGetTypeID() ); }
	inline bool isURL() const { return isOfType( CFURLGetTypeID() ); }
	inline bool isUUID() const { return isOfType( CFUUIDGetTypeID() ); }
  private:
	T obj = nullptr;
};

}

#endif

#endif
