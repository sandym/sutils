//
//  su_attachable.h
//  sutils
//
//  Created by Sandy Martel on 2013/09/22.
//  Copyright (c) 2015年 Sandy Martel. All rights reserved.
//
// Permission to use, copy, modify, distribute, and sell this software for any
// purpose is hereby granted without fee. The sotware is provided "AS-IS" and
// without warranty of any kind, express, implied or otherwise.

#ifndef H_SU_ATTACHABLE
#define H_SU_ATTACHABLE

#include <unordered_map>
#include <string>
#include <memory>

namespace su {

class attachment;

class attachable
{
public:
	attachable() = default;

	attachable( const attachable & ) = delete;
	attachable &operator=( const attachable & ) = delete;

	virtual ~attachable() = default;

	// ownership is transfered
	attachment *attach( const std::string &i_name,
						std::unique_ptr<attachment> &&i_attachment );

	// will delete attachment with that name
	void detach( const std::string &i_name );

	attachment *getRaw( const std::string &i_name ) const
	{
		auto it = _attachments.find( i_name );
		return it != _attachments.end() ? it->second.get() : nullptr;
	}

	template <class T>
	T *get( const std::string &i_name ) const
	{
		return dynamic_cast<T *>( getRaw( i_name ) );
	}

private:
	std::unordered_map<std::string,std::unique_ptr<attachment>> _attachments;
};

class attachment
{
public:
	attachment( const attachment & ) = delete;
	attachment &operator=( const attachment & ) = delete;

	attachment() = default;
	virtual ~attachment() = default;

protected:
	class attachable *attachable() { return _attachable; }
	const class attachable *attachable() const { return _attachable; }

private:
	class attachable *_attachable = nullptr;

	friend class attachable;
};
}

#endif
