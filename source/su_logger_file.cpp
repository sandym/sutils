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
#include <chrono>
#include <string>

namespace {

/*! roll a file if it exists
		will rename "file" to "file_YYYY-MM-DD" or "file_YYYY-MM-DD-INDEX"
*/
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
		struct tm tmdata;
		char isoTime[32] = "";
#if UPLATFORM_WIN
		if ( localtime_s( &tmdata, &tm ) == 0 )
			strftime(isoTime, 32, "%Y-%m-%d", &tmdata);
#else
		strftime( isoTime, 32, "%Y-%m-%d", localtime_r( &tm, &tmdata ) );
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

//! logger_output that tee to 2 streams
struct tee_output : public su::logger_output
{
	tee_output( std::ostream &ostr1, std::ostream &ostr2 )
		: logger_output( ostr1 ),
			tee( ostr1.rdbuf(), ostr2.rdbuf() )
	{
		_save = ostr.rdbuf( &tee );
	}
	~tee_output()
	{
		ostr.rdbuf( _save );
	}
	
	su::teebuf tee;
	std::streambuf *_save = nullptr;
};

//! helper that close, roll and re-open a new file daily
struct RollDailyHelper
{
	RollDailyHelper( const su::filepath &i_path, std::ofstream &ofstr )
		: _path( i_path ), _ofstr( ofstr )
	{
		timeout = nextMidnight();
	}
	
	const su::filepath _path;
	std::ofstream &_ofstr;
	std::chrono::system_clock::time_point timeout;
	
	void flush()
	{
		if ( std::chrono::system_clock::now() > timeout )
		{
			_ofstr.close();
			roll( _path );
			_path.fsopen( _ofstr );
			timeout = nextMidnight();
		}
	}
	
	std::chrono::system_clock::time_point nextMidnight()
	{
		// align to next midnight, local time.
		auto tt = std::chrono::system_clock::to_time_t( std::chrono::system_clock::now() );
	
		struct tm tmdata;
#if UPLATFORM_WIN
		localtime_s( &tmdata, &tt );
#else
		localtime_r( &tt, &tmdata );
#endif
		tmdata.tm_hour = 0;
		tmdata.tm_min = 0;
		tmdata.tm_sec = 0;
		tmdata.tm_mday += 1;
		tt = mktime( &tmdata );

		return std::chrono::system_clock::from_time_t( tt );
	}
};

//! helper that close, roll and re-open a new file when a certain size is reach
struct RollOnSizeHelper
{
	RollOnSizeHelper( const su::filepath &i_path, std::ofstream &ofstr, int i_bytes )
		: _path( i_path ), _ofstr( ofstr ), _bytes( i_bytes ) {}
	
	const su::filepath _path;
	std::ofstream &_ofstr;
	const int _bytes;;
	
	void flush()
	{
		if ( _ofstr and _ofstr.tellp() >= _bytes )
		{
			_ofstr.close();
			roll( _path );
			_path.fsopen( _ofstr );
		}
	}
};

}

namespace su {

logger_file::logger_file( logger_base &i_logger, const filepath &i_path, const Append &, bool i_tee )
	: _logger( i_logger )
{
	i_path.fsopen( _fstr, std::ios::app );
	_save = _logger.exchangeOutput( createSimpleStream( i_tee ) );
}

logger_file::logger_file( logger_base &i_logger, const filepath &i_path, const Overwrite &, bool i_tee )
	: _logger( i_logger )
{
	i_path.fsopen( _fstr );
	_save = _logger.exchangeOutput( createSimpleStream( i_tee ) );
}

logger_file::logger_file( logger_base &i_logger, const filepath &i_path, const Roll &, bool i_tee )
	: _logger( i_logger )
{
	roll( i_path );
	i_path.fsopen( _fstr );
	_save = _logger.exchangeOutput( createSimpleStream( i_tee ) );
}

logger_file::logger_file( logger_base &i_logger, const filepath &i_path, const RollDaily &, bool i_tee )
	: _logger( i_logger )
{
	roll( i_path );
	i_path.fsopen( _fstr );
	_save = _logger.exchangeOutput( createRollDailyStream( i_path, i_tee ) );
}

logger_file::logger_file( logger_base &i_logger, const filepath &i_path, const RollOnSize &i_action, bool i_tee )
	: _logger( i_logger )
{
	roll( i_path );
	i_path.fsopen( _fstr );
	_save = _logger.exchangeOutput( createRollOnSizeStream( i_path, i_tee, i_action.bytes ) );
}

logger_file::~logger_file()
{
	// restore the logger
	_logger.exchangeOutput( std::move(_save) );
}

std::unique_ptr<logger_output> logger_file::createSimpleStream( bool i_tee )
{
	if ( i_tee and _logger.output() != nullptr )
		return std::make_unique<tee_output>( _fstr, _logger.output()->ostr );
	
	return std::make_unique<logger_output>( _fstr );
}

std::unique_ptr<logger_output> logger_file::createRollDailyStream( const filepath &i_path, bool i_tee )
{
	if ( i_tee and _logger.output() != nullptr )
	{
		// compose tee and roll daily features
		struct output : public tee_output, public RollDailyHelper
		{
			output( const filepath &i_path, std::ofstream &ofstr, std::ostream &ostr2 )
				: tee_output( ofstr, ostr2 ), RollDailyHelper( i_path, ofstr ) {}
			
			virtual void flush()
			{
				logger_output::flush();
				RollDailyHelper::flush();
			}
		};
		return std::make_unique<output>( i_path, _fstr, _logger.output()->ostr );
	}

	// roll daily output
	struct output : public logger_output, public RollDailyHelper
	{
		output( const filepath &i_path, std::ofstream &ofstr )
			: logger_output( ofstr ), RollDailyHelper( i_path, ofstr ) {}
		
		virtual void flush()
		{
			logger_output::flush();
			RollDailyHelper::flush();
		}
	};
	return std::make_unique<output>( i_path, _fstr );
}

std::unique_ptr<logger_output> logger_file::createRollOnSizeStream( const filepath &i_path, bool i_tee, int i_bytes )
{
	if ( i_tee and _logger.output() != nullptr )
	{
		// compose tee and roll on size features
		struct output : public tee_output, public RollOnSizeHelper
		{
			output( const filepath &i_path, std::ofstream &ofstr, std::ostream &ostr2, int i_bytes )
				: tee_output( ofstr, ostr2 ), RollOnSizeHelper( i_path, ofstr, i_bytes ) {}
			
			virtual void flush()
			{
				logger_output::flush();
				RollOnSizeHelper::flush();
			}
		};
		return std::make_unique<output>( i_path, _fstr, _logger.output()->ostr, i_bytes );
	}

	// roll on size output
	struct output : public logger_output, public RollOnSizeHelper
	{
		output( const filepath &i_path, std::ofstream &ofstr, int i_bytes )
			: logger_output( ofstr ), RollOnSizeHelper( i_path, ofstr, i_bytes ){}
		
		virtual void flush()
		{
			logger_output::flush();
			RollOnSizeHelper::flush();
		}
	};
	return std::make_unique<output>( i_path, _fstr, i_bytes );
}

}
