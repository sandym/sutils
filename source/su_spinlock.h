/*
 *  su_spinlock.h
 *  sutils
 *
 *  Created by Sandy Martel on 2015/09/03.
 *  Copyright (c) 2015年 Sandy Martel. All rights reserved.
 *
 * Permission to use, copy, modify, distribute, and sell this software for any purpose is hereby
 * granted without fee. The sotware is provided "AS-IS" and without warranty of any kind, express,
 * implied or otherwise.
 */

#ifndef H_SU_SPINLOCK
#define H_SU_SPINLOCK

#include <atomic>

namespace su {

class spinlock
{
private:
	std::atomic_flag _locked = ATOMIC_FLAG_INIT;
	
public:
	inline void lock()
	{
		while ( _locked.test_and_set(std::memory_order_acquire) );
	}
	
	inline void unlock()
	{
		_locked.clear(std::memory_order_release);
	}
};

}

#endif
