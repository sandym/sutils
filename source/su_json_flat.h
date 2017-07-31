//
//  su_json_flat.h
//  sutils
//
//  Created by Sandy Martel on 17/05/05.
//  Copyright (c) 2015年 Sandy Martel. All rights reserved.
//
//	Permission to use, copy, modify, distribute, and sell this software for any purpose is hereby
//	granted without fee. The sotware is provided "AS-IS" and without warranty of any kind, express,
//	implied or otherwise.
//

#ifndef H_SU_JSON_FLAT
#define H_SU_JSON_FLAT

#include "su_json.h"
#include "shim/optional.h"

namespace su {

void flatten( const su::Json &i_json, std::ostream &ostr );
su::optional<su::Json> unflatten( std::istream &istr );

}

#endif
