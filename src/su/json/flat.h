//
//  flat.h
//  sutils
//
//  Created by Sandy Martel on 17/11/10.
//  Copyright (c) 2017年 Sandy Martel. All rights reserved.
//
// Permission to use, copy, modify, distribute, and sell this software for any
// purpose is hereby granted without fee. The sotware is provided "AS-IS" and
// without warranty of any kind, express, implied or otherwise.
//

#ifndef H_SU_JSON_FLAT
#define H_SU_JSON_FLAT

#include "su/json/json.h"
#include "su/containers/array_view.h"

namespace su {
namespace flat {

std::string write( const su::Json &i_json );
su::Json read( const array_view<char> &i_buffer );

} }

#endif
