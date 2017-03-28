/*
 *  su_logger.h
 *  sutils
 *
 *  Created by Sandy Martel on 08-04-20.
 *  Copyright (c) 2015年 Sandy Martel. All rights reserved.
 *
 * Permission to use, copy, modify, distribute, and sell this software for any purpose is hereby
 * granted without fee. The sotware is provided "AS-IS" and without warranty of any kind, express,
 * implied or otherwise.
 */

/*
    usage:
        su::logger.setLogMask( mask )

        log_trace({subsystem}) << args ... ;
        log_debug({subsystem}) << args ... ;
        log_info({subsystem}) << args ... ;
        log_warn({subsystem}) << args ... ;
        log_error({subsystem}) << args ... ;
        log_fault({subsystem}) << args ... ;
		
		Note that subsystem is optional

*/

#ifndef H_SU_LOGGER
#define H_SU_LOGGER

#include <ciso646>
#include <iosfwd>
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include "su_always_inline.h"

namespace su
{
// from less to more verbose
enum loglevel
{
	kFAULT = 0x01,
	kERROR = 0x02,
	kWARN = 0x04,
	kINFO = 0x08,
	kDEBUG = 0x10,
	kTRACE = 0x20
};

// level not in kCOMPILETIME_LOG_MASK will be compiled out!
#ifndef COMPILETIME_LOG_MASK
#ifdef NDEBUG
// in release, all but debug and trace
const int kCOMPILETIME_LOG_MASK = ~(kDEBUG + kTRACE);
#else
// in debug, all
const int kCOMPILETIME_LOG_MASK = 0xFF;
#endif
#else
const int kCOMPILETIME_LOG_MASK = COMPILETIME_LOG_MASK;
#undef COMPILETIME_LOG_MASK
#endif

//! @todo: remove once in std
struct source_location
{
	source_location() = default;
	source_location(const char *i_file, int i_line, const char *i_func)
		: _file(i_file), _line(i_line), _func(i_func) {}

    const char *_file = nullptr;
    int _line = -1;
    const char *_func = nullptr;
    
    inline const char *function_name() const { return _func; }
    inline const char *file_name() const { return _file; }
    inline int line() const { return _line; }
};

class LogEvent final
{
  private:
	const static int kInlineBufferSize = 128;
	
	// event serialisation
	char _inlineBuffer[kInlineBufferSize];
	std::unique_ptr<char []> _heapBuffer;
	size_t _capacity = kInlineBufferSize;
	size_t _size = 0;
	
	void ensure_capacity( size_t s );
	
	void encode_string_data( const char *i_data, size_t s );
	void encode_string_literal( const char *i_data );
	template<typename T>
	void encode( const T &v );
	
	inline char *buffer() { return _capacity > kInlineBufferSize ? _heapBuffer.get() : _inlineBuffer; }
	inline const char *buffer() const { return _capacity > kInlineBufferSize ? _heapBuffer.get() : _inlineBuffer; }
	
	// info
	int _level;
	source_location _sl;
	std::thread::id _threadID;
	uint64_t _time;
	
  public:
	LogEvent( const LogEvent & ) = delete;
	LogEvent &operator=( const LogEvent & ) = delete;

	LogEvent( LogEvent &&lhs ) = default;
	LogEvent &operator=( LogEvent &&lhs ) = default;
	
	LogEvent( int i_level );
	LogEvent( int i_level, su::source_location &&i_sl );

	LogEvent &operator<<( bool v );
	LogEvent &operator<<( char v );
	LogEvent &operator<<( unsigned char v );
	LogEvent &operator<<( short v );
	LogEvent &operator<<( unsigned short v );
	LogEvent &operator<<( int v );
	LogEvent &operator<<( unsigned int v );
	LogEvent &operator<<( long v );
	LogEvent &operator<<( unsigned long v );
	LogEvent &operator<<( long long v );
	LogEvent &operator<<( unsigned long long v );
	LogEvent &operator<<( double v );

	inline LogEvent &operator<<( const std::string &v )
	{
		encode_string_data( v.c_str(), v.size() );
		return *this;
	}

	template<size_t N>
	LogEvent &operator<<( const char (&v)[N] )
	{
		encode_string_literal( v );
		return *this;
	}
	
	template<typename T>
	inline typename std::enable_if<std::is_same<T,const char *>::value,LogEvent &>::type
	operator<<( const T &v )
	{
		encode_string_data( v, strlen( v ) );
		return *this;
	}
	template<typename T>
	inline typename std::enable_if<std::is_same<T,char *>::value,LogEvent &>::type
	operator<<( const T &v )
	{
		encode_string_data( v, strlen( v ) );
		return *this;
	}
	
	void dump( const std::string &i_ss, std::ostream &ostr ) const;
};

class LoggerBase
{
  protected:
	LoggerBase( std::ostream &ostr, const std::string &i_subsystem );
	virtual ~LoggerBase();

	std::ostream &_ostr;
	std::string _subsystem;
	
  public:
	inline std::ostream &ostr() const { return _ostr; }
	inline const std::string &subsystem() const { return _subsystem; }

	bool operator==( LogEvent &i_event );
};

template <int COMPILETIME_LOG_MASK=kCOMPILETIME_LOG_MASK>
class Logger final : public LoggerBase
{
  public:
	Logger( std::ostream &ostr, const std::string &i_ss = std::string() ) : LoggerBase( ostr, i_ss ){}
	Logger( const Logger & ) = delete;
	Logger &operator=( const Logger & ) = delete;
	
	int getLogMask() const { return _runtimeLogMask; }
	void setLogMask( int l ) { _runtimeLogMask = l & COMPILETIME_LOG_MASK; }
	
	template<int LEVEL>
	inline bool shouldLog() const { return (COMPILETIME_LOG_MASK&LEVEL) != 0 and (_runtimeLogMask&LEVEL) != 0; }

  private:
	int _runtimeLogMask = COMPILETIME_LOG_MASK;
};

extern Logger<kCOMPILETIME_LOG_MASK> logger; // default logger

always_inline_func su::Logger<kCOMPILETIME_LOG_MASK> &GET_LOGGER() { return su::logger; }
template<int L>
always_inline_func su::Logger<L> &GET_LOGGER( su::Logger<L> &i_logger ) { return i_logger; }

#define log_fault(...) su::GET_LOGGER(__VA_ARGS__).shouldLog<su::kFAULT>() and su::GET_LOGGER(__VA_ARGS__) == su::LogEvent( su::kFAULT, {__FILE__,__LINE__,__FUNCTION__})
#define log_error(...) su::GET_LOGGER(__VA_ARGS__).shouldLog<su::kERROR>() and su::GET_LOGGER(__VA_ARGS__) == su::LogEvent( su::kERROR, {__FILE__,__LINE__,__FUNCTION__})
#define log_warn(...) su::GET_LOGGER(__VA_ARGS__).shouldLog<su::kWARN>() and su::GET_LOGGER(__VA_ARGS__) == su::LogEvent( su::kWARN, {__FILE__,__LINE__,__FUNCTION__})
#define log_info(...) su::GET_LOGGER(__VA_ARGS__).shouldLog<su::kINFO>() and su::GET_LOGGER(__VA_ARGS__) == su::LogEvent( su::kINFO, {__FILE__,__LINE__,__FUNCTION__})
#define log_debug(...) su::GET_LOGGER(__VA_ARGS__).shouldLog<su::kDEBUG>() and su::GET_LOGGER(__VA_ARGS__) == su::LogEvent( su::kDEBUG, {__FILE__,__LINE__,__FUNCTION__})
#define log_trace(...) su::GET_LOGGER(__VA_ARGS__).shouldLog<su::kTRACE>() and su::GET_LOGGER(__VA_ARGS__) == su::LogEvent( su::kTRACE, {__FILE__,__LINE__,__FUNCTION__})

class logger_thread final
{
  public:
	logger_thread( const logger_thread & ) = delete;
	logger_thread &operator=( const logger_thread & ) = delete;
	
	logger_thread();
	~logger_thread();
	
	void flush();
};

}

#endif
