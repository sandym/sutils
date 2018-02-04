/*
 *  logger.cpp
 *  sutils
 *
 *  Created by Sandy Martel on 08-04-20.
 *  Copyright (c) 2015年 Sandy Martel. All rights reserved.
 *
 * Permission to use, copy, modify, distribute, and sell this software for any
 * purpose is hereby granted without fee. The sotware is provided "AS-IS" and
 * without warranty of any kind, express, implied or otherwise.
 */

#include "su/log/logger.h"
#include "su/base/platform.h"
#include "su/concurrency/spinlock.h"
#include "su/concurrency/thread.h"
#include <string.h>
#include <algorithm>
#include <cassert>
#include <condition_variable>
#include <ctime>
#include <iostream>
#include <sstream>
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
struct thread_name
{
	std::thread::native_handle_type threadId;
	thread_name( std::thread::native_handle_type i_threadId ) :
	    threadId( i_threadId )
	{
	}
};
std::ostream &operator<<( std::ostream &os, const thread_name &t )
{
#if UPLATFORM_WIN
	typedef HRESULT( WINAPI * GetThreadDescriptionPtr )( HANDLE, PWSTR * );

	auto GetThreadDescriptionFunc =
	    reinterpret_cast<GetThreadDescriptionPtr>(::GetProcAddress(
	        ::GetModuleHandleW( L"Kernel32.dll" ), "GetThreadDescription" ) );
	if ( GetThreadDescriptionFunc )
	{
		wchar_t *name = nullptr;
		if ( SUCCEEDED( GetThreadDescriptionFunc( t.threadId, &name ) ) )
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
			{
				buffer[l] = 0;
				return os << buffer;
			}
		}
	}
#else
	char buffer[20];
	if ( pthread_getname_np( t.threadId, buffer, 20 ) == 0 and buffer[0] != 0 )
		return os << buffer;
#endif
	return os << t.threadId;
}

class logger_thread_data
{
private:
	int refCount = 1; // not protected, but typically only one logger_thread
	                  // should be created
	std::mutex queueMutex;
	std::condition_variable queueCond;
	su::spinlock queueSpinLock;
	std::thread t;
	std::vector<RecordedEvent> logQueue;

	bool logQueueIsEmpty();

	void func();

public:
	logger_thread_data();

	inline void inc() { ++refCount; }
	void dec();

	void push( su::logger_base *i_logger, su::log_event &&i_event );
	void flush();
};
logger_thread_data *g_thread = nullptr;

logger_thread_data::logger_thread_data()
{
	assert( g_thread == nullptr );
	t = std::thread( &logger_thread_data::func, this );
	su::set_low_priority( t );
}

void logger_thread_data::dec()
{
	--refCount;
	if ( refCount == 0 )
	{
		// push kill message
		std::unique_lock<su::spinlock> l( queueSpinLock );
		logQueue.emplace_back( RecordedEvent{} );
		l.unlock();
		queueCond.notify_one();

		// and wait
		t.join();
		delete g_thread;
		g_thread = nullptr;
	}
}

bool logger_thread_data::logQueueIsEmpty()
{
	std::unique_lock<su::spinlock> l( queueSpinLock );
	return logQueue.empty();
}

void logger_thread_data::push(
    su::logger_base *i_logger,
    su::log_event &&i_event )
{
	std::unique_lock<su::spinlock> l( queueSpinLock );
	logQueue.emplace_back(
	    RecordedEvent{i_logger, std::move( i_event )} );
	l.unlock();
	queueCond.notify_one();
}

void logger_thread_data::flush()
{
	if ( not logQueueIsEmpty() )
	{
		std::unique_lock<std::mutex> l( queueMutex );
		queueCond.notify_one();
		queueCond.wait( l, [this]() { return this->logQueueIsEmpty(); } );
	}
}

void logger_thread_data::func()
{
	su::this_thread::set_name( "logger_thread" );

	std::unique_lock<std::mutex> l( queueMutex, std::defer_lock );

	std::vector<RecordedEvent> localCopy;
	std::unordered_set<su::logger_base *> toFlush;

	bool exitLoop = false;
	while ( not exitLoop )
	{
		l.lock();
		// wait for logs
		queueCond.wait( l, [this]() { return not this->logQueueIsEmpty(); } );
		l.unlock();

		// quickly grab a local copy
		std::unique_lock<su::spinlock> sl( queueSpinLock );
		localCopy.swap( logQueue );
		sl.unlock();

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
		queueCond.notify_all();
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

std::string log_event::message() const
{
	// [TIME][LEVEL][thread][file:func:line] msg

	int state = 0;
	
	unsigned long long us;
	int level;
	std::thread::native_handle_type threadId;
	const char *file_name = nullptr;
	const char *function_name = nullptr;
	int line;
	
	auto ptr = _buffer;
	auto end = _ptr;
	while ( ptr < end and state < 6 )
	{
		auto t = *reinterpret_cast<const log_data_type *>( ptr );
		ptr += sizeof( log_data_type );
		switch ( state )
		{
			case 0: // time
			{
				if ( t != log_data_type::kUnsignedLongLong )
					return {};
				us = *reinterpret_cast<const unsigned long long *>( ptr );
				ptr += sizeof( unsigned long long );
				break;
			}
			case 1: // level
			{
				if ( t != log_data_type::kInt )
					return {};
				level = *reinterpret_cast<const int *>( ptr );
				ptr += sizeof( int );
				break;
			}
			case 2: // thread
			{
				if ( t != TypeToEnum<uintptr_t>::value )
					return {};
				threadId = (std::thread::native_handle_type)*reinterpret_cast<const uintptr_t *>( ptr );
				ptr += sizeof( uintptr_t );
				break;
			}
			case 3: // file
			{
				if ( t != log_data_type::kStringLiteral )
					return {};
				file_name = *reinterpret_cast<const char *const *>( ptr );
				ptr += sizeof( const char * );
				break;
			}
			case 4: // func
			{
				if ( t != log_data_type::kStringLiteral )
					return {};
				function_name = *reinterpret_cast<const char *const *>( ptr );
				ptr += sizeof( const char * );
				break;
			}
			case 5: // line
			{
				if ( t != log_data_type::kInt )
					return {};
				line = *reinterpret_cast<const int *>( ptr );
				ptr += sizeof( int );
				break;
			}
		}
		++state;
	}
	if ( state < 6 )
		return {};
	
	const char *levelStr = "";
	switch ( level )
	{
		case su::kFAULT:
			levelStr = "FAULT";
			break;
		case su::kERROR:
			levelStr = "ERROR";
			break;
		case su::kWARN:
			levelStr = "WARN";
			break;
		case su::kINFO:
			levelStr = "INFO";
			break;
		case su::kDEBUG:
			levelStr = "DEBUG";
			break;
		case su::kTRACE:
			levelStr = "TRACE";
			break;
		default:
			return {};
	}

	std::time_t t = us / 1000000;
	char isoTime[32] = "";
	char ms[8] = "";
	struct tm tmdata;
#if UPLATFORM_WIN
	if ( localtime_s( &tmdata, &t ) == 0 )
	{
		strftime( isoTime, 32, "%Y-%m-%dT%T", &tmdata );
		sprintf_s( ms, ".%06lu", (unsigned long)( us % 1000000 ) );
	}
#else
	strftime( isoTime, 32, "%Y-%m-%dT%T", localtime_r( &t, &tmdata ) );
	sprintf( ms, ".%06lu", (unsigned long)( us % 1000000 ) );
#endif

	std::ostringstream ostr;

	ostr << "[" << isoTime << ms << "][" << levelStr << "]["
	     << thread_name( threadId ) << "]";

	// output location, if available
	if ( *file_name != 0 )
	{
		//	just write the basename
		std::string_view file{file_name};
		auto pos = file.find_last_of( UPLATFORM_WIN ? '\\' : '/' );
		if ( pos != std::string_view::npos )
			file = file.substr( pos + 1 );
		ostr << "[" << file;
		if ( *function_name != 0 )
			ostr << ":" << function_name;
		if ( line != -1 )
			ostr << ":" << line;
		ostr << "]";
	}
	else if ( *function_name != 0 )
		ostr << "[" << function_name << "]";

	ostr << " ";

	while ( ptr < end )
	{
		auto t = *reinterpret_cast<const log_data_type *>( ptr );
		ptr += sizeof( log_data_type );
		switch ( t )
		{
			case log_data_type::kBool:
				if ( *reinterpret_cast<const bool *>( ptr ) )
					ostr << "true";
				else
					ostr << "false";
				ptr += sizeof( bool );
				break;
			case log_data_type::kChar:
				ostr << *reinterpret_cast<const char *>( ptr );
				ptr += sizeof( char );
				break;
			case log_data_type::kUnsignedChar:
				ostr << (int)*reinterpret_cast<const unsigned char *>( ptr );
				ptr += sizeof( unsigned char );
				break;
			case log_data_type::kShort:
				ostr << *reinterpret_cast<const short *>( ptr );
				ptr += sizeof( short );
				break;
			case log_data_type::kUnsignedShort:
				ostr << *reinterpret_cast<const unsigned short *>( ptr );
				ptr += sizeof( unsigned short );
				break;
			case log_data_type::kInt:
				ostr << *reinterpret_cast<const int *>( ptr );
				ptr += sizeof( int );
				break;
			case log_data_type::kUnsignedInt:
				ostr << *reinterpret_cast<const unsigned int *>( ptr );
				ptr += sizeof( unsigned int );
				break;
			case log_data_type::kLong:
				ostr << *reinterpret_cast<const long *>( ptr );
				ptr += sizeof( long );
				break;
			case log_data_type::kUnsignedLong:
				ostr << *reinterpret_cast<const unsigned long *>( ptr );
				ptr += sizeof( unsigned long );
				break;
			case log_data_type::kLongLong:
				ostr << *reinterpret_cast<const long long *>( ptr );
				ptr += sizeof( long long );
				break;
			case log_data_type::kUnsignedLongLong:
				ostr << *reinterpret_cast<const unsigned long long *>( ptr );
				ptr += sizeof( unsigned long long );
				break;
			case log_data_type::kDouble:
				ostr << *reinterpret_cast<const double *>( ptr );
				ptr += sizeof( double );
				break;
			case log_data_type::kStringLiteral:
				ostr << *reinterpret_cast<const char *const *>( ptr );
				ptr += sizeof( const char * );
				break;
			case log_data_type::kStringData:
				ostr << reinterpret_cast<const char *>( ptr );
				ptr += strlen( reinterpret_cast<const char *>( ptr ) ) + 1;
				break;
			default:
				assert( false );
				break;
		}
	}
	return ostr.str();
}

Logger<kCOMPILETIME_LOG_MASK> logger( std::clog );

void logger_output::writeEvent( const log_event &i_event )
{
	auto msg = i_event.message();
	write( msg.data(), msg.size() );
	write( "\n", 1 );
}
void logger_output::write( const char *i_text, size_t l )
{
	_out.write( i_text, l );
}
void logger_output::flush()
{
	_out.flush();
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
