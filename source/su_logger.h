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
#include <string_view>
#include <thread>
#include "su_always_inline.h"

namespace su {

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
// in release, all but debug
const int kCOMPILETIME_LOG_MASK = ~kDEBUG;
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

	const char *function_name() const { return _func; }
	const char *file_name() const { return _file; }
	int line() const { return _line; }
};

//! record on log event
class log_event final
{
private:
	const static int kInlineBufferSize = 128;
	
	// event serialisation
	char _inlineBuffer[kInlineBufferSize]; //!< initial buffer, stack allocated
	char *_buffer = _inlineBuffer; //!< ptr to the buffer in use
	char *_ptr = _buffer; //!< current position in the buffer
	std::unique_ptr<char []> _heapBuffer; //!< if more space is needed, heap allocated
	size_t _capacity = kInlineBufferSize; //!< current capacity
	
	void ensure_extra_capacity( size_t extra );
	
	void encode_string_data( const char *i_data, size_t s );
	void encode_string_literal( const char *i_data );
	template<typename T>
	void encode( const T &v );
	
	// info
	int _level;
	source_location _sl;
	std::thread::native_handle_type _threadID;
	uint64_t _timestamp;
	
public:
	log_event( const log_event & ) = delete;
	log_event &operator=( const log_event & ) = delete;

	log_event( log_event &&lhs ) = default;
	log_event &operator=( log_event &&lhs ) = default;
	
	log_event( int i_level );
	log_event( int i_level, su::source_location &&i_sl );

	log_event &operator<<( bool v );
	log_event &operator<<( char v );
	log_event &operator<<( unsigned char v );
	log_event &operator<<( short v );
	log_event &operator<<( unsigned short v );
	log_event &operator<<( int v );
	log_event &operator<<( unsigned int v );
	log_event &operator<<( long v );
	log_event &operator<<( unsigned long v );
	log_event &operator<<( long long v );
	log_event &operator<<( unsigned long long v );
	log_event &operator<<( double v );

	log_event &operator<<( const std::string_view &v )
	{
		encode_string_data( v.data(), v.size() );
		return *this;
	}

	template<size_t N>
	log_event &operator<<( char (&v)[N] )
	{
		encode_string_data( v, N - 1 );
		return *this;
	}

	template<size_t N>
	log_event &operator<<( const char (&v)[N] )
	{
		encode_string_literal( v );
		return *this;
	}
	
	template<typename T>
	typename std::enable_if_t<std::is_same<std::remove_cv_t<T>,char *>::value,log_event &>
	operator<<( const T &v )
	{
		encode_string_data( v, strlen( v ) );
		return *this;
	}
	
	int level() const { return _level; }
	uint64_t timestamp() const { return _timestamp; }
	std::thread::native_handle_type threadID() const { return _threadID; }
	const source_location &sl() const { return _sl; }
	
	std::string message() const;
	std::ostream &message( std::ostream &ostr ) const;
};

struct logger_output
{
	logger_output( std::ostream &s ) : ostr(s){}
	virtual ~logger_output() = default;
	
	std::ostream &ostr;
	virtual void flush();
};

class logger_base
{
protected:
	logger_base( std::unique_ptr<logger_output> &&i_output, const std::string &i_subsystem );
	virtual ~logger_base();

	std::unique_ptr<logger_output> _output;
	std::string _subsystem;
	
public:
	logger_base( const logger_base & ) = delete;
	logger_base &operator=( const logger_base & ) = delete;

	std::unique_ptr<logger_output> exchangeOutput( std::unique_ptr<logger_output> &&i_output );

	logger_output *output() const { return _output.get(); }
	const std::string &subsystem() const { return _subsystem; }

	// don't call those directly.
	bool operator==( log_event &i_event );
	void dump( const log_event &i_event );
};

template <int COMPILETIME_LOG_MASK=kCOMPILETIME_LOG_MASK>
class Logger final : public logger_base
{
public:
	Logger( const std::string &i_ss = {} ) : logger_base( {}, i_ss ){}
	Logger( std::unique_ptr<logger_output> &&i_output, const std::string &i_ss = {} )
		: logger_base( std::move(i_output), i_ss ){}
	Logger( std::ostream &ostr, const std::string &i_ss = {} )
		: logger_base( std::make_unique<logger_output>(ostr), i_ss ){}
	
	int getLogMask() const { return _runtimeLogMask; }
	void setLogMask( int l ) { _runtimeLogMask = l & COMPILETIME_LOG_MASK; }
	
	template<int LEVEL>
	bool shouldLog() const { return (COMPILETIME_LOG_MASK&LEVEL) != 0 and (_runtimeLogMask&LEVEL) != 0; }

private:
	int _runtimeLogMask = COMPILETIME_LOG_MASK;
};

extern Logger<kCOMPILETIME_LOG_MASK> logger; //!< the default logger

always_inline_func su::Logger<kCOMPILETIME_LOG_MASK> &GET_LOGGER() { return su::logger; }
template<int L>
always_inline_func su::Logger<L> &GET_LOGGER( su::Logger<L> &i_logger ) { return i_logger; }

#define log_fault(...) su::GET_LOGGER(__VA_ARGS__).shouldLog<su::kFAULT>() and su::GET_LOGGER(__VA_ARGS__) == su::log_event(su::kFAULT,{__FILE__,__LINE__,__FUNCTION__})
#define log_error(...) su::GET_LOGGER(__VA_ARGS__).shouldLog<su::kERROR>() and su::GET_LOGGER(__VA_ARGS__) == su::log_event(su::kERROR,{__FILE__,__LINE__,__FUNCTION__})
#define log_warn(...) su::GET_LOGGER(__VA_ARGS__).shouldLog<su::kWARN>() and su::GET_LOGGER(__VA_ARGS__) == su::log_event(su::kWARN,{__FILE__,__LINE__,__FUNCTION__})
#define log_info(...) su::GET_LOGGER(__VA_ARGS__).shouldLog<su::kINFO>() and su::GET_LOGGER(__VA_ARGS__) == su::log_event(su::kINFO,{__FILE__,__LINE__,__FUNCTION__})
#define log_debug(...) su::GET_LOGGER(__VA_ARGS__).shouldLog<su::kDEBUG>() and su::GET_LOGGER(__VA_ARGS__) == su::log_event(su::kDEBUG,{__FILE__,__LINE__,__FUNCTION__})
#define log_trace(...) su::GET_LOGGER(__VA_ARGS__).shouldLog<su::kTRACE>() and su::GET_LOGGER(__VA_ARGS__) == su::log_event(su::kTRACE,{__FILE__,__LINE__,__FUNCTION__})

//! instanciate one of those to defer the logging output to a thread
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
