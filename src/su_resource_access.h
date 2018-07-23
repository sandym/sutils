/*
 *  su_resource_access.cpp
 *  sutils
 *
 *  Created by Sandy Martel on 08-09-07.
 *  Copyright (c) 2015年 Sandy Martel. All rights reserved.
 *
 * Permission to use, copy, modify, distribute, and sell this software for any
 * purpose is hereby granted without fee. The sotware is provided "AS-IS" and
 * without warranty of any kind, express, implied or otherwise.
 */

#ifndef H_SU_RESOURCE_ACCESS
#define H_SU_RESOURCE_ACCESS

#include "su_filepath.h"

namespace su {
namespace resource_access {

su::filepath getFolder();
su::filepath get( const std::string_view &i_name );

}
}

#endif
