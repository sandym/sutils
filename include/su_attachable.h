//
//  su_attachable.h
//  sutils
//
//  Created by Sandy Martel on 2013/09/22.
//  Copyright (c) 2015年 Sandy Martel. All rights reserved.
//
// Permission to use, copy, modify, distribute, and sell this software for any
// purpose is hereby
// granted without fee. The sotware is provided "AS-IS" and without warranty of
// any kind, express,
// implied or otherwise.

#ifndef H_SU_ATTACHABLE
#define H_SU_ATTACHABLE

#include "su_flat_map.h"
#include <string>

namespace su
{
class attachment;

class attachable
{
  public:
	attachable() = default;

	attachable( const attachable & ) = delete;
	attachable &operator=( const attachable & ) = delete;

	virtual ~attachable();

	// ownership is transfered
	void attach( const std::string &i_name, attachment *i_attachment );

	// will delete attachment with that name
	void detach( const std::string &i_name );

	inline attachment *getRaw( const std::string &i_name ) const
	{
		auto it = _attachments.find( i_name );
		return it != _attachments.end() ? it->second : nullptr;
	}

	template <class T>
	T *get( const std::string &i_name ) const
	{
		return dynamic_cast<T *>( getRaw( i_name ) );
	}

  private:
	su::flat_map<std::string, attachment *> _attachments;

	void detach( attachment *i_attachment );

	friend class attachment;
};

class attachment
{
  public:
	attachment( const attachment & ) = delete;
	attachment &operator=( const attachment & ) = delete;

	attachment() = default;
	virtual ~attachment();

  protected:
	inline class attachable *attachable() { return _attachable; }
	inline const class attachable *attachable() const { return _attachable; }
  private:
	class attachable *_attachable = nullptr;

	friend class attachable;
};
}

#endif