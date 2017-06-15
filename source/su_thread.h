//
//  su_thread.h
//  sutils
//
//  Created by Sandy Martel on 12/03/10.
//  Copyright (c) 2015年 Sandy Martel. All rights reserved.
//
// Permission to use, copy, modify, distribute, and sell this software for any purpose is hereby
// granted without fee. The sotware is provided "AS-IS" and without warranty of any kind, express,
// implied or otherwise.
//

#ifndef H_SU_THREAD
#define H_SU_THREAD

#include <thread>
#include <cassert>

namespace su {

namespace this_thread {

// extension
void set_as_main();
bool is_main();
void set_name( const char *n );

}

void set_low_priority( std::thread &i_thread );

}

#ifdef NDEBUG

#define AssertIsMainThread()
#define AssertIsNotMainThread()

#else

//! macro to assert which thread we're on
#define AssertIsMainThread() assert( su::this_thread::is_main() )
#define AssertIsNotMainThread() assert( not su::this_thread::is_main() )

#endif

#endif
