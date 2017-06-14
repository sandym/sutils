/*
 *  su_logger_file.cpp
 *  sutils
 *
 *  Created by Sandy Martel on 06-01-27.
 *  Copyright (c) 2015年 Sandy Martel. All rights reserved.
 *
 * Permission to use, copy, modify, distribute, and sell this software for any purpose is hereby
 * granted without fee. The sotware is provided "AS-IS" and without warranty of any kind, express,
 * implied or otherwise.
 */

#include "su_logger_file.h"
#include "su_filepath.h"
#include "su_teebuf.h"
#include <cassert>

namespace {

void roll( const su::filepath &i_path )
{
	auto tm = i_path.creation_date();
	if ( tm > 0 )
	{
		auto ext = i_path.extension();
		if ( not ext.empty() )
			ext.insert( 0, "." );
		auto newName = i_path.stem();
		newName += "_";
		struct tm gmtm;
		char isoTime[32] = "";
#if UPLATFORM_WIN
		if (gmtime_s(&gmtm, &tm) == 0)
			strftime(isoTime, 32, "%Y-%m-%d", &gmtm);
#else
		strftime( isoTime, 32, "%Y-%m-%d", gmtime_r( &tm, &gmtm ) );
#endif
		newName += isoTime;
		su::filepath folder( i_path );
		folder.up();
		su::filepath pushTo( folder );
		pushTo.add( newName + ext );
		newName += "-";
		int i = 0;
		while ( pushTo.exists() )
		{
			pushTo = folder;
			pushTo.add( newName + std::to_string( ++i ) + ext );
		}
		i_path.move( pushTo );
	}
}

struct file_logger_output : public su::logger_output
{
	file_logger_output( const su::filepath &i_path,
						std::ios_base::openmode i_mode = std::ios_base::out | std::ios_base::trunc |
															std::ios_base::binary )
		: su::logger_output( fstr )
	{
		i_path.fsopen( fstr, i_mode );
	}
	
	std::ofstream fstr;
};

struct file_logger_output_roll_daily : public file_logger_output
{
	file_logger_output_roll_daily( const su::filepath &i_path,
						std::ios_base::openmode i_mode = std::ios_base::out | std::ios_base::trunc |
															std::ios_base::binary )
		: file_logger_output( i_path, i_mode )
	{}

	virtual void flush()
	{
		su::logger_output::flush();
		assert( false );
	}
};

struct file_logger_output_roll_on_size : public file_logger_output
{
	file_logger_output_roll_on_size( int i_bytes, const su::filepath &i_path,
						std::ios_base::openmode i_mode = std::ios_base::out | std::ios_base::trunc |
															std::ios_base::binary )
		: file_logger_output( i_path, i_mode )
	{}

	virtual void flush()
	{
		su::logger_output::flush();
		assert( false );
	}
};

struct tee_file_logger_output : public su::logger_output
{
	tee_file_logger_output( std::ostream &other,
							const su::filepath &i_path,
							std::ios_base::openmode i_mode = std::ios_base::out | std::ios_base::trunc |
																std::ios_base::binary )
		: su::logger_output( fstr ), _tee( other.rdbuf(), fstr.rdbuf() )
	{
		i_path.fsopen( fstr, i_mode );
		_save = ostr.rdbuf( &_tee );
	}
	~tee_file_logger_output()
	{
		ostr.rdbuf( _save );
	}
	
	std::ofstream fstr;
	su::teebuf _tee;
	std::streambuf *_save = nullptr;
};

struct tee_file_logger_output_roll_daily : public tee_file_logger_output
{
	tee_file_logger_output_roll_daily( std::ostream &other,
							const su::filepath &i_path,
							std::ios_base::openmode i_mode = std::ios_base::out | std::ios_base::trunc |
																std::ios_base::binary )
		: tee_file_logger_output( other, i_path, i_mode )
	{}
	
	virtual void flush()
	{
		su::logger_output::flush();
		assert( false );
	}
};

struct tee_file_logger_output_roll_on_size : public tee_file_logger_output
{
	tee_file_logger_output_roll_on_size( int i_bytes, std::ostream &other,
							const su::filepath &i_path,
							std::ios_base::openmode i_mode = std::ios_base::out | std::ios_base::trunc |
																std::ios_base::binary )
		: tee_file_logger_output( other, i_path, i_mode )
	{}
	
	virtual void flush()
	{
		su::logger_output::flush();
		assert( false );
	}
};


}

namespace su {

logger_file::logger_file( logger_base &i_logger, const filepath &i_path, const Append &, bool i_tee )
	: _logger( i_logger )
{
	std::unique_ptr<logger_output> lo;
	if ( i_tee and _logger.output() != nullptr )
	{
		lo = std::make_unique<tee_file_logger_output>( _logger.output()->ostr, i_path, std::ios::app );
	}
	else
	{
		lo = std::make_unique<file_logger_output>( i_path, std::ios::app );
	}
	_saveOutput = _logger.exchangeOutput( std::move(lo) );
}

logger_file::logger_file( logger_base &i_logger, const filepath &i_path, const Overwrite &, bool i_tee )
	: _logger( i_logger )
{
	std::unique_ptr<logger_output> lo;
	if ( i_tee and _logger.output() != nullptr )
	{
		lo = std::make_unique<tee_file_logger_output>( _logger.output()->ostr, i_path );
	}
	else
	{
		lo = std::make_unique<file_logger_output>( i_path );
	}
	_saveOutput = _logger.exchangeOutput( std::move(lo) );
}

logger_file::logger_file( logger_base &i_logger, const filepath &i_path, const Roll &, bool i_tee )
	: _logger( i_logger )
{
	roll( i_path );
	
	std::unique_ptr<logger_output> lo;
	if ( i_tee and _logger.output() != nullptr )
	{
		lo = std::make_unique<tee_file_logger_output>( _logger.output()->ostr, i_path );
	}
	else
	{
		lo = std::make_unique<file_logger_output>( i_path );
	}
	_saveOutput = _logger.exchangeOutput( std::move(lo) );
}

logger_file::logger_file( logger_base &i_logger, const filepath &i_path, const RollDaily &, bool i_tee )
	: _logger( i_logger )
{
	roll( i_path );

	std::unique_ptr<logger_output> lo;
	if ( i_tee and _logger.output() != nullptr )
	{
		lo = std::make_unique<tee_file_logger_output_roll_daily>( _logger.output()->ostr, i_path );
	}
	else
	{
		lo = std::make_unique<file_logger_output_roll_daily>( i_path );
	}
	_saveOutput = _logger.exchangeOutput( std::move(lo) );
}

logger_file::logger_file( logger_base &i_logger, const filepath &i_path, const RollOnSize &i_action, bool i_tee )
	: _logger( i_logger )
{
	roll( i_path );

	std::unique_ptr<logger_output> lo;
	if ( i_tee and _logger.output() != nullptr )
	{
		lo = std::make_unique<tee_file_logger_output_roll_on_size>( i_action.bytes, _logger.output()->ostr, i_path );
	}
	else
	{
		lo = std::make_unique<file_logger_output_roll_on_size>( i_action.bytes, i_path );
	}
	_saveOutput = _logger.exchangeOutput( std::move(lo) );
}


logger_file::~logger_file()
{
	_logger.exchangeOutput( std::move(_saveOutput) );
}

}
