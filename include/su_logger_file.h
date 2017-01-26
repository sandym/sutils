/*
 *  ulogger_file.h
 *  sutils
 *
 *  Created by Sandy Martel on 08-04-20.
 *  Copyright (c) 2015年 Sandy Martel. All rights reserved.
 *
 * Permission to use, copy, modify, distribute, and sell this software for any purpose is hereby
 * granted without fee. The sotware is provided "AS-IS" and without warranty of any kind, express,
 * implied or otherwise.
 */

#ifndef H_ULOGGER_FILE
#define H_ULOGGER_FILE

#include "su_teebuf.h"
#include <fstream>
#include <memory>

namespace su
{
class LoggerBase;
class filepath;

class logger_file final
{
  public:
	enum class action
	{
		kAppend, //!< append to the current file
		kOverwrite, //!< overwrite the current file
		kPush //!< rename the current file using its creation date and start a new file
	};

	// redirect i_logger to a file
	logger_file( LoggerBase &i_logger, const filepath &i_path, bool i_tee, action i_action );

	// redirect the default logger to a file
	logger_file( const filepath &i_path, bool i_tee, action i_action );
	~logger_file();

	logger_file( const logger_file & ) = delete;
	logger_file &operator=( const logger_file & ) = delete;

  private:
	logger_file( std::ostream &i_sink, const filepath &i_path, bool i_tee, action i_action );

	std::ofstream _ofstr;
	std::streambuf *_save = nullptr;
	std::ostream &_sink;
	std::unique_ptr<su::teebuf> _tee;
};
}

#endif
