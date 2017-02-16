/*
 *  ulogger_file.cpp
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
#include "su_logger.h"
#include "su_filepath.h"

namespace su
{
logger_file::logger_file( std::ostream &i_sink, const filepath &i_path, bool i_tee, action i_action ) : _sink( i_sink )
{
	switch ( i_action )
	{
		case action::kAppend:
			i_path.fsopen( _ofstr, std::ios::app );
			break;
		case action::kOverwrite:
			i_path.fsopen( _ofstr );
			break;
		case action::kPush:
		{
			auto tm = i_path.creation_date();
			if ( tm > 0 )
			{
				auto ext = i_path.extension();
				if ( not ext.empty() )
					ext.insert( 0, "." );
				auto newName = i_path.name( false );
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
				filepath folder( i_path );
				folder.up();
				filepath pushTo( folder );
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
			i_path.fsopen( _ofstr );
			break;
		}
	}

	_save = _sink.rdbuf();
	if ( i_tee )
	{
		_tee.reset( new su::teebuf( _sink.rdbuf(), _ofstr.rdbuf() ) );
		_sink.rdbuf( _tee.get() );
	}
	else
		_sink.rdbuf( _ofstr.rdbuf() );
}

logger_file::logger_file( LoggerBase &i_logger, const filepath &i_path, bool i_tee, action i_action )
	: logger_file( i_logger.ostr(), i_path, i_tee, i_action )
{
}

logger_file::logger_file( const filepath &i_path, bool i_tee, logger_file::action i_action )
	: logger_file( std::clog, i_path, i_tee, i_action )
{
}

logger_file::~logger_file()
{
	// restore
	_sink.flush();
	_sink.rdbuf( _save );
}
}
