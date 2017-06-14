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
#include <chrono>

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

}

namespace su {

logger_file::logger_file( logger_base &i_logger, const filepath &i_path, const Append &, bool i_tee )
	: _logger( i_logger )
{
	i_path.fsopen( _fstr, std::ios::app );
	setStream( i_tee );
}

logger_file::logger_file( logger_base &i_logger, const filepath &i_path, const Overwrite &, bool i_tee )
	: _logger( i_logger )
{
	i_path.fsopen( _fstr );
	setStream( i_tee );
}

logger_file::logger_file( logger_base &i_logger, const filepath &i_path, const Roll &, bool i_tee )
	: _logger( i_logger )
{
	roll( i_path );
	i_path.fsopen( _fstr );
	setStream( i_tee );
}

logger_file::logger_file( logger_base &i_logger, const filepath &i_path, const RollDaily &, bool i_tee )
	: _logger( i_logger )
{
	roll( i_path );
	i_path.fsopen( _fstr );
	setStreamRollDaily( i_path, i_tee );
}

logger_file::logger_file( logger_base &i_logger, const filepath &i_path, const RollOnSize &i_action, bool i_tee )
	: _logger( i_logger )
{
	roll( i_path );
	i_path.fsopen( _fstr );
	setStreamRollOnSize( i_path, i_tee, i_action.bytes );
}

logger_file::~logger_file()
{
	_logger.exchangeOutput( std::move(_save) );
}

void logger_file::setStream( bool i_tee )
{
	if ( i_tee and _logger.output() != nullptr )
	{
		struct output : public logger_output
		{
			output( std::ostream &ostr1, std::ostream &ostr2 )
				: logger_output( ostr1 ),
					tee( ostr1.rdbuf(), ostr2.rdbuf() )
			{
				_save = ostr.rdbuf( &tee );
			}
			~output()
			{
				ostr.rdbuf( _save );
			}
			
			teebuf tee;
			std::streambuf *_save = nullptr;
		};
		_save = _logger.exchangeOutput( std::make_unique<output>( _fstr, _logger.output()->ostr ) );
	}
	else
	{
		_save = _logger.exchangeOutput( std::make_unique<logger_output>( _fstr ) );
	}
}

void logger_file::setStreamRollDaily( const filepath &i_path, bool i_tee )
{
	if ( i_tee and _logger.output() != nullptr )
	{
		struct output : public logger_output
		{
			output( const filepath &i_path, std::ofstream &ofstr, std::ostream &ostr2 )
				: logger_output( ofstr ),
					tee( ofstr.rdbuf(), ostr2.rdbuf() ),
					_path( i_path ),
					_ofstr( ofstr )
			{
				_save = ostr.rdbuf( &tee );
				timeout = std::chrono::system_clock::now() + std::chrono::hours(24);
			}
			~output()
			{
				ostr.rdbuf( _save );
			}
			
			teebuf tee;
			std::streambuf *_save = nullptr;
			const filepath _path;
			std::ofstream &_ofstr;
			std::chrono::system_clock::time_point timeout;
			
			virtual void flush()
			{
				logger_output::flush();
				if ( std::chrono::system_clock::now() > timeout )
				{
					_ofstr.close();
					roll( _path );
					_path.fsopen( _ofstr );
					timeout = std::chrono::system_clock::now() + std::chrono::hours(24);
				}
			}
		};
		_save = _logger.exchangeOutput( std::make_unique<output>( i_path, _fstr, _logger.output()->ostr ) );
	}
	else
	{
		struct output : public logger_output
		{
			output( const filepath &i_path, std::ofstream &ofstr )
				: logger_output( ofstr ),
					_path( i_path ),
					_ofstr( ofstr )
			{
				timeout = std::chrono::system_clock::now() + std::chrono::hours(24);
			}
			
			const filepath _path;
			std::ofstream &_ofstr;
			std::chrono::system_clock::time_point timeout;
			
			virtual void flush()
			{
				logger_output::flush();
				if ( std::chrono::system_clock::now() > timeout )
				{
					_ofstr.close();
					roll( _path );
					_path.fsopen( _ofstr );
					timeout = std::chrono::system_clock::now() + std::chrono::hours(24);
				}
			}
		};
		_save = _logger.exchangeOutput( std::make_unique<output>( i_path, _fstr ) );
	}
}

void logger_file::setStreamRollOnSize( const filepath &i_path, bool i_tee, int i_bytes )
{
	if ( i_tee and _logger.output() != nullptr )
	{
		struct output : public logger_output
		{
			output( const filepath &i_path, std::ofstream &ofstr, std::ostream &ostr2, int i_bytes )
				: logger_output( ofstr ),
					tee( ofstr.rdbuf(), ostr2.rdbuf() ),
					_path( i_path ),
					_ofstr( ofstr ),
					_bytes( i_bytes )
			{
				_save = ostr.rdbuf( &tee );
			}
			~output()
			{
				ostr.rdbuf( _save );
			}
			
			teebuf tee;
			std::streambuf *_save = nullptr;
			const filepath _path;
			std::ofstream &_ofstr;
			const int _bytes;;
			
			virtual void flush()
			{
				logger_output::flush();
				if ( _ofstr.tellp() >= _bytes )
				{
					_ofstr.close();
					roll( _path );
					_path.fsopen( _ofstr );
				}
			}
		};
		_save = _logger.exchangeOutput( std::make_unique<output>( i_path, _fstr, _logger.output()->ostr, i_bytes ) );
	}
	else
	{
		struct output : public logger_output
		{
			output( const filepath &i_path, std::ofstream &ofstr, int i_bytes )
				: logger_output( ofstr ),
					_path( i_path ),
					_ofstr( ofstr ),
					_bytes( i_bytes )
			{}
			
			const filepath _path;
			std::ofstream &_ofstr;
			const int _bytes;;
			
			virtual void flush()
			{
				logger_output::flush();
				if ( _ofstr.tellp() >= _bytes )
				{
					_ofstr.close();
					roll( _path );
					_path.fsopen( _ofstr );
				}
			}
		};
		_save = _logger.exchangeOutput( std::make_unique<output>( i_path, _fstr, i_bytes ) );
	}
}

}
