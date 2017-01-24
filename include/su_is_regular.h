/*
 *  su_is_regular.h
 *  sutils
 *
 *  Created by Sandy Martel on 2015/07/08.
 *  Copyright (c) 2015年 Sandy Martel. All rights reserved.
 *
 * Permission to use, copy, modify, distribute, and sell this software for any purpose is hereby
 * granted without fee. The sotware is provided "AS-IS" and without warranty of any kind, express,
 * implied or otherwise.
 */

#ifndef H_SU_IS_REGULAR
#define H_SU_IS_REGULAR

#include <type_traits>
#include <ciso646>

namespace su
{

template<typename T>
struct is_regular :
    std::integral_constant<bool,
        std::is_default_constructible<T>::value and
        std::is_copy_constructible<T>::value and
        std::is_move_constructible<T>::value and
        std::is_copy_assignable<T>::value and
        std::is_move_assignable<T>::value>
     {};
//struct T {};
//static_assert(is_regular<T>::value, "huh?");

}

#endif
