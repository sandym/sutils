/*
 *  su_logger_file.h
 *  sutils
 *
 *  Created by Sandy Martel on 08-04-20.
 *  Copyright (c) 2015年 Sandy Martel. All rights reserved.
 *
 * Permission to use, copy, modify, distribute, and sell this software for any
 * purpose is hereby granted without fee. The sotware is provided "AS-IS" and
 * without warranty of any kind, express, implied or otherwise.
 */

#ifndef H_SU_LOGGER_FILE
#define H_SU_LOGGER_FILE

#include "su_logger.h"
#include <fstream>

namespace su {

class filepath;

class logger_file final
{
public:
	//! append to the current file
	struct Append {};
	//! overwrite the current file
	struct Overwrite {};
	//! rename the current file using its creation date and start a new file
	struct Roll {};
	struct RollDaily {};
	struct RollOnSize
	{
		int bytes = 10 * 1024 * 1024;
	};

	logger_file( logger_base &i_logger,
	             const filepath &i_path,
	             const Append &,
	             bool i_tee );
	logger_file( logger_base &i_logger,
	             const filepath &i_path,
	             const Overwrite &,
	             bool i_tee );
	logger_file( logger_base &i_logger,
	             const filepath &i_path,
	             const Roll &,
	             bool i_tee );
	logger_file( logger_base &i_logger,
	             const filepath &i_path,
	             const RollDaily &,
	             bool i_tee );
	logger_file( logger_base &i_logger,
	             const filepath &i_path,
	             const RollOnSize &,
	             bool i_tee );

	template<typename ACTION>
	logger_file( const filepath &i_path, const ACTION &i_action, bool i_tee ) :
	    logger_file( su::logger, i_path, i_action, i_tee )
	{
	}

	~logger_file();

private:
	std::ofstream _fstr;
	logger_base &_logger;
	std::unique_ptr<logger_output> _save;

	std::unique_ptr<logger_output> createSimpleStream( bool i_tee );
	std::unique_ptr<logger_output> createRollDailyStream(
	    const filepath &i_path, bool i_tee );
	std::unique_ptr<logger_output> createRollOnSizeStream(
	    const filepath &i_path, bool i_tee, int i_bytes );
};

}

#endif
