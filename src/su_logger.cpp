/*
 *  su_logger.cpp
 *  sutils
 *
 *  Created by Sandy Martel on 08-04-20.
 *  Copyright (c) 2015年 Sandy Martel. All rights reserved.
 *
 * Permission to use, copy, modify, distribute, and sell this software for any
 * purpose is hereby granted without fee. The sotware is provided "AS-IS" and
 * without warranty of any kind, express, implied or otherwise.
 */

#include "su_logger.h"
#include "su_platform.h"
#include "su_thread.h"
#include <string.h>
#include <algorithm>
#include <cassert>
#include <condition_variable>
#include <ctime>
#include <iostream>
#include <unordered_set>
#include <vector>

#if UPLATFORM_WIN
#	include <windows.h>
#endif

namespace {

// type-to-enum to help encode event data
enum class log_data_type : uint8_t
{
	kBool,
	kChar,
	kUnsignedChar,
	kShort,
	kUnsignedShort,
	kInt,
	kUnsignedInt,
	kLong,
	kUnsignedLong,
	kLongLong,
	kUnsignedLongLong,
	kDouble,
	kStringData,
	kStringLiteral
};
template<typename T>
struct TypeToEnum
{
};
template<>
struct TypeToEnum<bool>
{
	static const log_data_type value = log_data_type::kBool;
};
template<>
struct TypeToEnum<char>
{
	static const log_data_type value = log_data_type::kChar;
};
template<>
struct TypeToEnum<unsigned char>
{
	static const log_data_type value = log_data_type::kUnsignedChar;
};
template<>
struct TypeToEnum<short>
{
	static const log_data_type value = log_data_type::kShort;
};
template<>
struct TypeToEnum<unsigned short>
{
	static const log_data_type value = log_data_type::kUnsignedShort;
};
template<>
struct TypeToEnum<int>
{
	static const log_data_type value = log_data_type::kInt;
};
template<>
struct TypeToEnum<unsigned int>
{
	static const log_data_type value = log_data_type::kUnsignedInt;
};
template<>
struct TypeToEnum<long>
{
	static const log_data_type value = log_data_type::kLong;
};
template<>
struct TypeToEnum<unsigned long>
{
	static const log_data_type value = log_data_type::kUnsignedLong;
};
template<>
struct TypeToEnum<long long>
{
	static const log_data_type value = log_data_type::kLongLong;
};
template<>
struct TypeToEnum<unsigned long long>
{
	static const log_data_type value = log_data_type::kUnsignedLongLong;
};
template<>
struct TypeToEnum<double>
{
	static const log_data_type value = log_data_type::kDouble;
};

struct RecordedEvent
{
	su::logger_base *logger = nullptr;
	su::log_event event{-1, {}};
};

// helper to print thread's name
std::string thread_name( std::thread::native_handle_type i_threadId )
{
#if UPLATFORM_WIN
	typedef HRESULT( WINAPI * GetThreadDescriptionPtr )( HANDLE, PWSTR * );

	static auto GetThreadDescriptionFunc =
	    reinterpret_cast<GetThreadDescriptionPtr>(::GetProcAddress(
	        ::GetModuleHandleW( L"Kernel32.dll" ), "GetThreadDescription" ) );
	if ( GetThreadDescriptionFunc and i_threadId )
	{
		wchar_t *name = nullptr;
		if ( SUCCEEDED( GetThreadDescriptionFunc( i_threadId, &name ) ) )
		{
			char buffer[64];
			int l = WideCharToMultiByte( CP_UTF8,
			                             WC_COMPOSITECHECK,
			                             name,
			                             wcslen( name ),
			                             buffer,
			                             64,
			                             nullptr,
			                             nullptr );
			LocalFree( name );
			if ( l > 0 )
				return std::string( buffer, l );
		}
	}
#else
	char buffer[20];
	if ( i_threadId and pthread_getname_np( i_threadId, buffer, 20 ) == 0 and
	     buffer[0] != 0 )
		return buffer;
#endif
	static const char *s_digits = "0123456789ABCDEF";
	uintptr_t threadId = (uintptr_t)i_threadId;
	const auto hex_len = sizeof( threadId ) << 1;
	std::string s( "0x" );
	for ( size_t i = 0, j = ( hex_len - 1 ) * 4; i < hex_len; ++i, j -= 4 )
		s.append( 1, s_digits[( threadId >> j ) & 0x0f] );
	return s;
}

class logger_thread_data
{
private:
	int _refCount = 1; // not protected, but typically only one logger_thread
	                  // should be created
	std::mutex _queueMutex;
	std::condition_variable _queueCond;
	std::thread _thread;
	std::vector<RecordedEvent> _logQueue;

	bool logQueueIsEmpty();

	void func();

public:
	logger_thread_data();

	void inc() { ++_refCount; }
	void dec();

	void push( su::logger_base *i_logger, su::log_event &&i_event );
	void flush();
};
logger_thread_data *g_thread = nullptr;

logger_thread_data::logger_thread_data()
{
	assert( g_thread == nullptr );
	_thread = std::thread( &logger_thread_data::func, this );
	su::set_low_priority( _thread );
}

void logger_thread_data::dec()
{
	--_refCount;
	if ( _refCount == 0 )
	{
		// push kill message
		std::unique_lock<std::mutex> l( _queueMutex );
		_logQueue.emplace_back( RecordedEvent{} );
		l.unlock();
		_queueCond.notify_one();

		// and wait
		_thread.join();
		delete g_thread;
		g_thread = nullptr;
	}
}

bool logger_thread_data::logQueueIsEmpty()
{
	std::unique_lock<std::mutex> l( _queueMutex );
	return _logQueue.empty();
}

void logger_thread_data::push( su::logger_base *i_logger,
                               su::log_event &&i_event )
{
	std::unique_lock<std::mutex> l( _queueMutex );
	_logQueue.emplace_back( RecordedEvent{i_logger, std::move( i_event )} );
	l.unlock();
	_queueCond.notify_one();
}

void logger_thread_data::flush()
{
	if ( not logQueueIsEmpty() )
	{
		std::unique_lock<std::mutex> l( _queueMutex );
		_queueCond.notify_one();
		_queueCond.wait( l, [this]() { return this->logQueueIsEmpty(); } );
	}
}

void logger_thread_data::func()
{
	su::this_thread::set_name( "logger_thread" );

	std::unique_lock<std::mutex> l( _queueMutex, std::defer_lock );

	std::vector<RecordedEvent> localCopy;
	std::unordered_set<su::logger_base *> toFlush;

	bool exitLoop = false;
	while ( not exitLoop )
	{
		l.lock();
		// wait for logs
		_queueCond.wait( l, [this]() { return not this->logQueueIsEmpty(); } );

		// quickly grab a local copy
		localCopy.swap( _logQueue );

		l.unlock();

		// dump all events
		for ( auto &rec : localCopy )
		{
			if ( rec.logger == nullptr )
				exitLoop = true;
			else if ( rec.logger->output() )
			{
				rec.logger->output()->writeEvent( rec.event );
				toFlush.insert( rec.logger );
			}
		}

		// flush all loggers that did some work
		for ( auto l : toFlush )
			l->output()->flush();

		toFlush.clear();
		localCopy.clear();

		// notify
		_queueCond.notify_all();
	}
}
}

namespace su {

log_event::log_event( int i_level )
{
	// [TIME][LEVEL][thread][file:func:line] msg
	auto us = std::chrono::duration_cast<std::chrono::microseconds>(
	              std::chrono::system_clock::now().time_since_epoch() )
	              .count();
	encode<unsigned long long>( us );
	encode( i_level );
#if UPLATFORM_WIN
	encode<uintptr_t>( (uintptr_t)GetCurrentThread() );
#else
	encode<uintptr_t>( (uintptr_t)pthread_self() );
#endif
	encode_string_literal( "" ); // file
	encode_string_literal( "" ); // func
	encode<int>( -1 ); // line
}

log_event::log_event( int i_level, const su::source_location &i_sl )
{
	// [TIME][LEVEL][thread][file:func:line] msg
	auto us = std::chrono::duration_cast<std::chrono::microseconds>(
	              std::chrono::system_clock::now().time_since_epoch() )
	              .count();
	encode<unsigned long long>( us );
	encode( i_level );
#if UPLATFORM_WIN
	encode<uintptr_t>( (uintptr_t)GetCurrentThread() );
#else
	encode<uintptr_t>( (uintptr_t)pthread_self() );
#endif
	encode_string_literal( i_sl.file_name() );
	encode_string_literal( i_sl.function_name() );
	encode<int>( i_sl.line() );
}

log_event::log_event( const dataView_t &i_data )
{
	ensure_extra_capacity( i_data.len );
	memcpy( _ptr, i_data.start, i_data.len );
	_ptr += i_data.len;
}

log_event::~log_event()
{
	if ( not storageIsInline() )
		delete[] _storage.heapBuffer.data;
}

log_event::log_event( log_event &&lhs )
{
	_storage = lhs._storage;
	_buffer = lhs._buffer;
	_ptr = lhs._ptr;

	lhs._buffer = lhs._storage.inlineBuffer;
	lhs._ptr = lhs._buffer;
}

log_event &log_event::operator=( log_event &&lhs )
{
	if ( this != &lhs )
	{
		_storage = lhs._storage;
		_buffer = lhs._buffer;
		_ptr = lhs._ptr;

		lhs._buffer = lhs._storage.inlineBuffer;
		lhs._ptr = lhs._buffer;
	}
	return *this;
}

void log_event::ensure_extra_capacity( size_t extra )
{
	auto currentSize = _ptr - _buffer;
	auto newSize = currentSize + extra;
	size_t cap =
	    storageIsInline() ? kInlineBufferSize : _storage.heapBuffer.capacity;
	if ( newSize > cap )
	{
		auto newCap = ( std::max )( newSize, cap * 2 );
		auto newBuffer = new char[newCap];
		memcpy( newBuffer, _buffer, currentSize );
		if ( not storageIsInline() )
			delete[] _storage.heapBuffer.data;

		_storage.heapBuffer.capacity = newCap;
		_storage.heapBuffer.data = newBuffer;
		_buffer = newBuffer;
		_ptr = _buffer + currentSize;
	}
}

template<typename T>
void log_event::encode( const T &v )
{
	ensure_extra_capacity( 1 + sizeof( v ) );
	*reinterpret_cast<log_data_type *>( _ptr ) = TypeToEnum<T>::value;
	++_ptr;
	*reinterpret_cast<T *>( _ptr ) = v;
	_ptr += sizeof( v );
}

log_event &log_event::operator<<( bool v )
{
	encode( v );
	return *this;
}
log_event &log_event::operator<<( char v )
{
	encode( v );
	return *this;
}
log_event &log_event::operator<<( unsigned char v )
{
	encode( v );
	return *this;
}
log_event &log_event::operator<<( short v )
{
	encode( v );
	return *this;
}
log_event &log_event::operator<<( unsigned short v )
{
	encode( v );
	return *this;
}
log_event &log_event::operator<<( int v )
{
	encode( v );
	return *this;
}
log_event &log_event::operator<<( unsigned int v )
{
	encode( v );
	return *this;
}
log_event &log_event::operator<<( long v )
{
	encode( v );
	return *this;
}
log_event &log_event::operator<<( unsigned long v )
{
	encode( v );
	return *this;
}
log_event &log_event::operator<<( long long v )
{
	encode( v );
	return *this;
}
log_event &log_event::operator<<( unsigned long long v )
{
	encode( v );
	return *this;
}
log_event &log_event::operator<<( double v )
{
	encode( v );
	return *this;
}

void log_event::encode_string_data( const char *i_data, size_t s )
{
	ensure_extra_capacity( sizeof( log_data_type ) + s + 1 );
	*reinterpret_cast<log_data_type *>( _ptr ) = log_data_type::kStringData;
	_ptr += sizeof( log_data_type );
	memcpy( _ptr, i_data, s + 1 );
	_ptr += s + 1;
}

void log_event::encode_string_literal( const char *i_data )
{
	ensure_extra_capacity( sizeof( log_data_type ) + sizeof( const char * ) );
	*reinterpret_cast<log_data_type *>( _ptr ) = log_data_type::kStringLiteral;
	_ptr += sizeof( log_data_type );
	*reinterpret_cast<const char **>( _ptr ) = i_data;
	_ptr += sizeof( const char * );
}

log_event::data_t log_event::extractData( char *&io_ptr ) const
{
	// [TIME][LEVEL][thread][file:func:line] msg
	data_t data;

	int state = 0;

	auto end = _ptr;
	while ( io_ptr < end and state < 6 )
	{
		auto t = *reinterpret_cast<const log_data_type *>( io_ptr );
		io_ptr += sizeof( log_data_type );
		switch ( state )
		{
			case 0: // time
			{
				if ( t != log_data_type::kUnsignedLongLong )
				{
					io_ptr -= sizeof( log_data_type );
					state = 6;
				}
				else
				{
					auto us =
					    *reinterpret_cast<const unsigned long long *>( io_ptr );
					io_ptr += sizeof( unsigned long long );

					std::time_t t = us / 1000000;
					char isoTime[32] = "";
					char ms[8] = "";
					struct tm tmdata;
#if UPLATFORM_WIN
					if ( localtime_s( &tmdata, &t ) == 0 )
					{
						strftime( isoTime, 32, "%Y-%m-%dT%T", &tmdata );
						sprintf_s(
						    ms, ".%06lu", (unsigned long)( us % 1000000 ) );
					}
#else
					strftime( isoTime,
					          32,
					          "%Y-%m-%dT%T",
					          localtime_r( &t, &tmdata ) );
					sprintf( ms, ".%06lu", (unsigned long)( us % 1000000 ) );
#endif
					strcpy( data.timestamp, isoTime );
					strcat( data.timestamp, ms );
				}
				break;
			}
			case 1: // level
			{
				if ( t != log_data_type::kInt )
				{
					io_ptr -= sizeof( log_data_type );
					state = 6;
				}
				else
				{
					auto level = *reinterpret_cast<const int *>( io_ptr );
					io_ptr += sizeof( int );
					switch ( level )
					{
						case su::kFAULT:
							data.level = "FAULT";
							break;
						case su::kERROR:
							data.level = "ERROR";
							break;
						case su::kWARN:
							data.level = "WARN";
							break;
						case su::kINFO:
							data.level = "INFO";
							break;
						case su::kDEBUG:
							data.level = "DEBUG";
							break;
						case su::kTRACE:
							data.level = "TRACE";
							break;
						default:
							break;
					}
				}
				break;
			}
			case 2: // thread
			{
				if ( t != TypeToEnum<uintptr_t>::value )
				{
					io_ptr -= sizeof( log_data_type );
					state = 6;
				}
				else
				{
					auto threadId =
					    ( std::thread::native_handle_type ) *
					    reinterpret_cast<const uintptr_t *>( io_ptr );
					io_ptr += sizeof( uintptr_t );
					data.threadId = thread_name( threadId );
				}
				break;
			}
			case 3: // file
			{
				if ( t != log_data_type::kStringLiteral )
				{
					io_ptr -= sizeof( log_data_type );
					state = 6;
				}
				else
				{
					data.file_name =
					    *reinterpret_cast<const char *const *>( io_ptr );
					io_ptr += sizeof( const char * );
				}
				break;
			}
			case 4: // func
			{
				if ( t != log_data_type::kStringLiteral )
				{
					io_ptr -= sizeof( log_data_type );
					state = 6;
				}
				else
				{
					data.function_name =
					    *reinterpret_cast<const char *const *>( io_ptr );
					io_ptr += sizeof( const char * );
				}
				break;
			}
			case 5: // line
			{
				if ( t != log_data_type::kInt )
				{
					io_ptr -= sizeof( log_data_type );
					state = 6;
				}
				else
				{
					data.line = *reinterpret_cast<const int *>( io_ptr );
					io_ptr += sizeof( int );
				}
				break;
			}
		}
		++state;
	}
	return data;
}

log_event::data_t log_event::getData() const
{
	auto ptr = _buffer;
	auto data = extractData( ptr );
	extractMessage( ptr, data.msg );
	return data;
}

std::string log_event::message() const
{
	// [TIME][LEVEL][thread][file:func:line] msg

	auto ptr = _buffer;
	auto data = extractData( ptr );

	std::string msg;
	msg.reserve( 256 );
	msg.append( 1, '[' );
	msg.append( data.timestamp );
	msg.append( "][" );
	msg.append( data.level );
	msg.append( "][" );
	msg.append( data.threadId );
	msg.append( 1, ']' );

	// output location, if available
	if ( data.file_name != nullptr and *data.file_name != 0 )
	{
		//	just write the basename
		std::string_view file{data.file_name};
		auto pos = file.find_last_of( UPLATFORM_WIN ? '\\' : '/' );
		if ( pos != std::string_view::npos )
			file = file.substr( pos + 1 );
		msg.append( 1, '[' );
		msg.append( file );
		if ( data.function_name != nullptr and *data.function_name != 0 )
		{
			msg.append( 1, ':' );
			msg.append( data.function_name );
		}
		if ( data.line != -1 )
		{
			msg.append( 1, ':' );
			msg.append( std::to_string( data.line ) );
		}
		msg.append( 1, ']' );
	}
	else if ( data.function_name != nullptr and *data.function_name != 0 )
	{
		msg.append( 1, '[' );
		msg.append( data.function_name );
		msg.append( 1, ']' );
	}

	msg.append( 1, ' ' );
	extractMessage( ptr, msg );
	return msg;
}

void log_event::extractMessage( char *&io_ptr, std::string &o_msg ) const
{
	auto end = _ptr;
	while ( io_ptr < end )
	{
		auto t = *reinterpret_cast<const log_data_type *>( io_ptr );
		io_ptr += sizeof( log_data_type );
		switch ( t )
		{
			case log_data_type::kBool:
				if ( *reinterpret_cast<const bool *>( io_ptr ) )
					o_msg.append( "true" );
				else
					o_msg.append( "false" );
				io_ptr += sizeof( bool );
				break;
			case log_data_type::kChar:
				o_msg.append( 1, *reinterpret_cast<const char *>( io_ptr ) );
				io_ptr += sizeof( char );
				break;
			case log_data_type::kUnsignedChar:
				o_msg.append( std::to_string(
				    (int)*reinterpret_cast<const unsigned char *>( io_ptr ) ) );
				io_ptr += sizeof( unsigned char );
				break;
			case log_data_type::kShort:
				o_msg.append( std::to_string(
				    *reinterpret_cast<const short *>( io_ptr ) ) );
				io_ptr += sizeof( short );
				break;
			case log_data_type::kUnsignedShort:
				o_msg.append( std::to_string(
				    *reinterpret_cast<const unsigned short *>( io_ptr ) ) );
				io_ptr += sizeof( unsigned short );
				break;
			case log_data_type::kInt:
				o_msg.append( std::to_string(
				    *reinterpret_cast<const int *>( io_ptr ) ) );
				io_ptr += sizeof( int );
				break;
			case log_data_type::kUnsignedInt:
				o_msg.append( std::to_string(
				    *reinterpret_cast<const unsigned int *>( io_ptr ) ) );
				io_ptr += sizeof( unsigned int );
				break;
			case log_data_type::kLong:
				o_msg.append( std::to_string(
				    *reinterpret_cast<const long *>( io_ptr ) ) );
				io_ptr += sizeof( long );
				break;
			case log_data_type::kUnsignedLong:
				o_msg.append( std::to_string(
				    *reinterpret_cast<const unsigned long *>( io_ptr ) ) );
				io_ptr += sizeof( unsigned long );
				break;
			case log_data_type::kLongLong:
				o_msg.append( std::to_string(
				    *reinterpret_cast<const long long *>( io_ptr ) ) );
				io_ptr += sizeof( long long );
				break;
			case log_data_type::kUnsignedLongLong:
				o_msg.append( std::to_string(
				    *reinterpret_cast<const unsigned long long *>( io_ptr ) ) );
				io_ptr += sizeof( unsigned long long );
				break;
			case log_data_type::kDouble:
				o_msg.append( std::to_string(
				    *reinterpret_cast<const double *>( io_ptr ) ) );
				io_ptr += sizeof( double );
				break;
			case log_data_type::kStringLiteral:
				o_msg.append(
				    *reinterpret_cast<const char *const *>( io_ptr ) );
				io_ptr += sizeof( const char * );
				break;
			case log_data_type::kStringData:
				o_msg.append( reinterpret_cast<const char *>( io_ptr ) );
				io_ptr +=
				    strlen( reinterpret_cast<const char *>( io_ptr ) ) + 1;
				break;
			default:
				assert( false );
				break;
		}
	}
}

Logger<kCOMPILETIME_LOG_MASK> logger( std::clog );

void logger_output::writeEvent( const log_event &i_event )
{
	auto msg = i_event.message();
	msg.append( 1, '\n' );
	ostr.write( msg.data(), msg.size() );
}
void logger_output::flush()
{
	ostr.flush();
}

logger_base::logger_base( std::unique_ptr<logger_output> &&i_output ) :
    _output( std::move( i_output ) )
{
}

logger_base::~logger_base()
{
	if ( g_thread != nullptr )
		g_thread->flush();
	else if ( _output )
		_output->flush();
}

bool logger_base::operator==( log_event &i_event )
{
	if ( g_thread != nullptr )
		g_thread->push( this, std::move( i_event ) );
	else if ( output() != nullptr )
	{
		output()->writeEvent( i_event );
	}
	return false;
}

std::unique_ptr<logger_output> logger_base::exchangeOutput(
    std::unique_ptr<logger_output> &&i_output )
{
	if ( g_thread != nullptr )
		g_thread->flush();
	else if ( _output )
		_output->flush();
	return std::exchange( _output, std::move( i_output ) );
}

logger_thread::logger_thread()
{
	if ( g_thread != nullptr )
		g_thread->inc();
	else
		g_thread = new logger_thread_data;
}

logger_thread::~logger_thread()
{
	assert( g_thread != nullptr );
	g_thread->dec();
}
}
