/*
 *  su_statesaver.h
 *  sutils
 *
 *  Created by Sandy Martel on 2014/07/08.
 *  Copyright (c) 2015年 Sandy Martel. All rights reserved.
 *
 * Permission to use, copy, modify, distribute, and sell this software for any
 * purpose is hereby  granted without fee. The sotware is provided "AS-IS" and
 * without warranty of any kind, express, implied or otherwise.
 */

#ifndef H_SU_STATESAVER
#define H_SU_STATESAVER

namespace su {

template<typename T>
class statesaver final
{
public:
	template<typename Y>
	statesaver( T &i_holder, const Y &v )
		: _holder( &i_holder ), _originalValue( i_holder )
	{
		*_holder = v;
	}
	statesaver( T &i_holder )
		: _holder( &i_holder ), _originalValue( i_holder )
	{
	}
	statesaver( const statesaver & ) = delete;
	statesaver &operator=( const statesaver & ) = delete;
	~statesaver()
	{
		restore();
	}

	void restore()
	{
		if ( _holder != nullptr )
		{
			*_holder = _originalValue;
			_holder = nullptr;
		}
	}

private:
	T *_holder;
	T _originalValue;
};

}

#endif
