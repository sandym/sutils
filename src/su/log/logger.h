/*
 *  logger.h
 *  sutils
 *
 *  Created by Sandy Martel on 08-04-20.
 *  Copyright (c) 2015年 Sandy Martel. All rights reserved.
 *
 * Permission to use, copy, modify, distribute, and sell this software for any
 * purpose is hereby granted without fee. The sotware is provided "AS-IS" and
 * without warranty of any kind, express, implied or otherwise.
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

#include "su/base/always_inline.h"
#include <string.h>
#include <ciso646>
#include <memory>
#include <string>
#include <string_view>
#include <thread>

#if __has_include("user_config.h")
#include "user_config.h"
#endif

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
#	ifdef NDEBUG
// in release, all but debug
const int kCOMPILETIME_LOG_MASK = ~kDEBUG;
#	else
// in debug, all
const int kCOMPILETIME_LOG_MASK = 0xFF;
#	endif
#else
const int kCOMPILETIME_LOG_MASK = COMPILETIME_LOG_MASK;
#	undef COMPILETIME_LOG_MASK
#endif

//! @todo: remove once in std
struct source_location
{
	source_location() = default;
	source_location( const char *i_file, int i_line, const char *i_func ) :
	    _file( i_file ),
	    _line( i_line ),
	    _func( i_func )
	{
	}

	const char *_file = nullptr;
	int _line = -1;
	const char *_func = nullptr;

	const char *function_name() const { return _func; }
	const char *file_name() const { return _file; }
	int line() const { return _line; }
};

//! record one log event
class log_event final
{
private:
	//! a heap buffer and capacity
	struct HeapBuffer
	{
		char *data;
		size_t capacity; //!< current capacity
	};

	//! event message storage: either inline in the object (no extra allocation)
	//  or on the heap
	const static int kInlineBufferSize = 128;
	union
	{
		char inlineBuffer[kInlineBufferSize]; //!< initial buffer, inline
		HeapBuffer heapBuffer; //!< if more space is needed, heap allocated
	} _storage;

	// buffer in use
	char *_buffer = _storage.inlineBuffer; //!< ptr to the buffer in use
	char *_ptr = _buffer; //!< current position in the buffer

	bool storageIsInline() const { return _buffer == _storage.inlineBuffer; }

	void ensure_extra_capacity( size_t extra );

	void encode_string_data( const char *i_data, size_t s );
	void encode_string_literal( const char *i_data );
	template<typename T>
	void encode( const T &v );

public:
	~log_event();

	log_event( const log_event & ) = delete;
	log_event &operator=( const log_event & ) = delete;

	log_event( log_event &&lhs );
	log_event &operator=( log_event &&lhs );

	log_event( int i_level );
	log_event( int i_level, const su::source_location &i_sl );

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
	log_event &operator<<( char ( &v )[N] )
	{
		encode_string_data( v, N - 1 );
		return *this;
	}

	template<size_t N>
	log_event &operator<<( const char ( &v )[N] )
	{
		encode_string_literal( v );
		return *this;
	}

	template<typename T>
	std::enable_if_t<std::is_same_v<std::remove_cv_t<T>, char *>, log_event &>
	operator<<( const T &v )
	{
		encode_string_data( v, strlen( v ) );
		return *this;
	}

	//! return a formatted line
	std::string message() const;

	//! accessor for the data
	struct data_t
	{
		char timestamp[40];
		const char *level = nullptr;
		std::string threadId;
		const char *file_name = nullptr;
		const char *function_name = nullptr;
		int line = -1;
		std::string msg;
	};
	data_t getData() const;

	//! accessor for the compact serialised data
	struct dataView_t
	{
		const char *start;
		size_t len;
	};
	dataView_t getDataView() const
	{
		return {_buffer, size_t( _ptr - _buffer )};
	}
	log_event( const dataView_t &i_data );

private:
	data_t extractData( char *&io_ptr ) const;
	void extractMessage( char *&io_ptr, std::string &o_msg ) const;
};

//! output for logs
struct logger_output
{
	logger_output( std::ostream &i_out ) : ostr( i_out ) {}
	virtual ~logger_output() = default;

	virtual void writeEvent( const log_event &i_event );
	virtual void flush();

	std::ostream &ostr;
};

class logger_base
{
protected:
	logger_base( std::unique_ptr<logger_output> &&i_output );
	virtual ~logger_base();

	std::unique_ptr<logger_output> _output;

public:
	logger_base( const logger_base & ) = delete;
	logger_base &operator=( const logger_base & ) = delete;

	std::unique_ptr<logger_output> exchangeOutput(
	    std::unique_ptr<logger_output> &&i_output );

	logger_output *output() const { return _output.get(); }

	// don't call those directly.
	bool operator==( log_event &i_event );
};

template<int COMPILETIME_LOG_MASK = kCOMPILETIME_LOG_MASK>
class Logger final : public logger_base
{
public:
	Logger() : logger_base( {} ) {}
	Logger( std::unique_ptr<logger_output> &&i_output ) :
	    logger_base( std::move( i_output ) )
	{
	}
	Logger( std::ostream &ostr ) :
	    logger_base( std::make_unique<logger_output>( ostr ) )
	{
	}

	int getLogMask() const { return _runtimeLogMask; }
	void setLogMask( int l ) { _runtimeLogMask = l & COMPILETIME_LOG_MASK; }

	template<int LEVEL>
	bool shouldLog() const
	{
		return ( COMPILETIME_LOG_MASK & LEVEL ) != 0 and
		       ( _runtimeLogMask & LEVEL ) != 0;
	}

private:
	int _runtimeLogMask = COMPILETIME_LOG_MASK;
};

extern Logger<kCOMPILETIME_LOG_MASK> logger; //!< the default logger

always_inline_func su::Logger<kCOMPILETIME_LOG_MASK> &GET_LOGGER()
{
	return su::logger;
}
template<int L>
always_inline_func su::Logger<L> &GET_LOGGER( su::Logger<L> &i_logger )
{
	return i_logger;
}

#define log_fault( ... )                                      \
	su::GET_LOGGER( __VA_ARGS__ ).shouldLog<su::kFAULT>() and \
	    su::GET_LOGGER( __VA_ARGS__ ) ==                      \
	        su::log_event( su::kFAULT, {__FILE__, __LINE__, __FUNCTION__} )
#define log_error( ... )                                      \
	su::GET_LOGGER( __VA_ARGS__ ).shouldLog<su::kERROR>() and \
	    su::GET_LOGGER( __VA_ARGS__ ) ==                      \
	        su::log_event( su::kERROR, {__FILE__, __LINE__, __FUNCTION__} )
#define log_warn( ... )                                      \
	su::GET_LOGGER( __VA_ARGS__ ).shouldLog<su::kWARN>() and \
	    su::GET_LOGGER( __VA_ARGS__ ) ==                     \
	        su::log_event( su::kWARN, {__FILE__, __LINE__, __FUNCTION__} )
#define log_info( ... )                                      \
	su::GET_LOGGER( __VA_ARGS__ ).shouldLog<su::kINFO>() and \
	    su::GET_LOGGER( __VA_ARGS__ ) ==                     \
	        su::log_event( su::kINFO, {__FILE__, __LINE__, __FUNCTION__} )
#define log_debug( ... )                                      \
	su::GET_LOGGER( __VA_ARGS__ ).shouldLog<su::kDEBUG>() and \
	    su::GET_LOGGER( __VA_ARGS__ ) ==                      \
	        su::log_event( su::kDEBUG, {__FILE__, __LINE__, __FUNCTION__} )
#define log_trace( ... )                                      \
	su::GET_LOGGER( __VA_ARGS__ ).shouldLog<su::kTRACE>() and \
	    su::GET_LOGGER( __VA_ARGS__ ) ==                      \
	        su::log_event( su::kTRACE, {__FILE__, __LINE__, __FUNCTION__} )

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
