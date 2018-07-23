/*
 *  su_always_inline.h
 *  sutils
 *
 *  Created by Sandy Martel on 2016/11/30.
 *  Copyright (c) 2016年 Sandy Martel. All rights reserved.
 *
 * Permission to use, copy, modify, distribute, and sell this software for any
 * purpose is hereby granted without fee. The sotware is provided "AS-IS" and
 * without warranty of any kind, express, implied or otherwise.
 */

#ifndef H_SU_ALWAYS_INLINE
#define H_SU_ALWAYS_INLINE

// MSVC and gcc/clang macros to force inline function
#ifdef _MSC_VER
#define always_inline_func __forceinline
#define never_inline_func __declspec(noinline)
#else
#define always_inline_func inline __attribute__((always_inline))
#define never_inline_func __attribute__((noinline))
#endif

#endif
