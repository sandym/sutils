/*
 *  platform.h
 *  sutils
 *
 *  Created by Sandy Martel on 06-03-16.
 *  Copyright (c) 2015年 Sandy Martel. All rights reserved.
 *
 * Permission to use, copy, modify, distribute, and sell this software for any
 * purpose is hereby granted without fee. The sotware is provided "AS-IS" and
 * without warranty of any kind, express, implied or otherwise.
 */

#ifndef H_SU_PLATFORM
#define H_SU_PLATFORM

//	defines those macros to 1 or 0
//		UPLATFORM_MAC
//		UPLATFORM_IOS
//		UPLATFORM_WIN
//		UPLATFORM_LINUX
//		UPLATFORM_UNIX
//		UPLATFORM_64BIT

#if defined(__APPLE_CC__)
#include <TargetConditionals.h>

//	GCC/llvm compiler on Apple platform
#	if TARGET_OS_IPHONE
#		define UPLATFORM_MAC		0
#		define UPLATFORM_IOS		1
#	else
#		define UPLATFORM_MAC		1
#		define UPLATFORM_IOS		0
#	endif

#	define UPLATFORM_WIN		0
#	define UPLATFORM_LINUX	0
#	define UPLATFORM_UNIX	1
#	ifdef __LP64__
#		define UPLATFORM_64BIT	1
#	else
#		define UPLATFORM_64BIT	0
#	endif

#elif defined(__linux__)

//	GCC compiler on linux

#	define UPLATFORM_MAC		0
#	define UPLATFORM_IOS		0
#	define UPLATFORM_WIN		0
#	define UPLATFORM_LINUX	1
#	define UPLATFORM_UNIX	1
#	ifdef __LP64__
#		define UPLATFORM_64BIT	1
#	else
#		define UPLATFORM_64BIT	0
#	endif

#elif defined(__GNUC__) && defined(__MINGW32__)

//	GCC compiler on windows with mingw32

#	define UPLATFORM_MAC		0
#	define UPLATFORM_IOS		0
#	define UPLATFORM_WIN		1
#	define UPLATFORM_LINUX	0
#	define UPLATFORM_UNIX	0
#	ifdef __LP64__
#		define UPLATFORM_64BIT	1
#	else
#		define UPLATFORM_64BIT	0
#	endif

#elif defined(_MSC_VER)

	//	VC

#	define UPLATFORM_MAC		0
#	define UPLATFORM_IOS		0
#	define UPLATFORM_WIN		1
#	define UPLATFORM_LINUX	0
#	define UPLATFORM_UNIX	0
#	ifdef _WIN64
#		define UPLATFORM_64BIT	1
#	else
#		define UPLATFORM_64BIT	0
#	endif

#else

#	error "unknown platform"

#endif

#endif
