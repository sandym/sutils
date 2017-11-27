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
#include "su/concurrency/spinlock.h"
#include "su/base/platform.h"
#include <unordered_set>
#include "su/concurrency/thread.h"
#include <iostream>
#include <ctime>
#include <condition_variable>
#include <cassert>
#include <sstream>
#include <algorithm>
#include <string.h>
#include <vector>

#if UPLATFORM_WIN
#include <windows.h>
#endif

namespace {

// type-to-enum to help encode event data
enum class log_data_type : uint8_t
{
	kBool,
	kChar, kUnsignedChar,
	kShort, kUnsignedShort,
	kInt, kUnsignedInt,
	kLong, kUnsignedLong,
	kLongLong, kUnsignedLongLong,
	kDouble,
	kStringData,
	kStringLiteral
};
template<typename T> struct TypeToEnum{};
template<> struct TypeToEnum<bool>{ static const log_data_type value = log_data_type::kBool; };
template<> struct TypeToEnum<char>{ static const log_data_type value = log_data_type::kChar; };
template<> struct TypeToEnum<unsigned char>{ static const log_data_type value = log_data_type::kUnsignedChar; };
template<> struct TypeToEnum<short>{ static const log_data_type value = log_data_type::kShort; };
template<> struct TypeToEnum<unsigned short>{ static const log_data_type value = log_data_type::kUnsignedShort; };
template<> struct TypeToEnum<int>{ static const log_data_type value = log_data_type::kInt; };
template<> struct TypeToEnum<unsigned int>{ static const log_data_type value = log_data_type::kUnsignedInt; };
template<> struct TypeToEnum<long>{ static const log_data_type value = log_data_type::kLong; };
template<> struct TypeToEnum<unsigned long>{ static const log_data_type value = log_data_type::kUnsignedLong; };
template<> struct TypeToEnum<long long>{ static const log_data_type value = log_data_type::kLongLong; };
template<> struct TypeToEnum<unsigned long long>{ static const log_data_type value = log_data_type::kUnsignedLongLong; };
template<> struct TypeToEnum<double>{ static const log_data_type value = log_data_type::kDouble; };

struct RecordedEvent
{
	su::logger_base *logger = nullptr;
	su::log_event event{ -1, {} };
	std::chrono::system_clock::time_point timestamp;
	std::thread::native_handle_type threadId;
};

// helper to print thread's name
struct thread_name
{
	std::thread::native_handle_type threadId;
	thread_name( std::thread::native_handle_type i_threadId ) : threadId( i_threadId ){}
};
std::ostream &operator<<( std::ostream &os, const thread_name &t )
{
#if UPLATFORM_WIN
	typedef HRESULT(WINAPI* GetThreadDescriptionPtr)( HANDLE, PWSTR* );

	auto GetThreadDescriptionFunc = reinterpret_cast<GetThreadDescriptionPtr>(
							::GetProcAddress( ::GetModuleHandleW(L"Kernel32.dll"), "GetThreadDescription" ) );
	if ( GetThreadDescriptionFunc )
	{
		wchar_t *name = nullptr;
		if ( SUCCEEDED(GetThreadDescriptionFunc( t.threadId, &name )) )
		{
			char buffer[64];
			int l = WideCharToMultiByte( CP_UTF8, WC_COMPOSITECHECK, name, wcslen(name), buffer, 64, nullptr, nullptr );
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

void dump( su::logger_base *i_logger, const std::chrono::system_clock::time_point &i_timestamp, std::thread::native_handle_type i_threadId, const su::log_event &i_event )
{
	if ( i_logger->output() == nullptr )
		return;
	
	// [TIME][LEVEL][thread][subsystem][file:func:line] msg
	
	const char *level = "";
	switch ( i_event.level() )
	{
		case su::kFAULT:
			level = "FAULT";
			break;
		case su::kERROR:
			level = "ERROR";
			break;
		case su::kWARN:
			level = "WARN";
			break;
		case su::kINFO:
			level = "INFO";
			break;
		case su::kDEBUG:
			level = "DEBUG";
			break;
		case su::kTRACE:
			level = "TRACE";
			break;
		default:
			return;
	}

	auto us = std::chrono::duration_cast<std::chrono::microseconds>(i_timestamp.time_since_epoch()).count();
	std::time_t t = us / 1000000;
	char isoTime[32] = "";
	char ms[8] = "";
	struct tm tmdata;
#if UPLATFORM_WIN
	if ( localtime_s( &tmdata, &t ) == 0 )
	{
		strftime(isoTime, 32, "%Y-%m-%dT%T", &tmdata);
		sprintf_s(ms, ".%06lu", (unsigned long)(us % 1000000));
	}
#else
	strftime( isoTime, 32, "%Y-%m-%dT%T", localtime_r( &t, &tmdata ) );
	sprintf(ms, ".%06lu", (unsigned long)(us % 1000000));
#endif
	
	auto &ostr = i_logger->output()->ostr;
	
	ostr << "[" << isoTime << ms << "][" << level << "][" << thread_name(i_threadId) << "]";
	if ( not i_logger->subsystem().empty() )
		ostr << "[" << i_logger->subsystem() << "]";

	// output location, if available
	if ( i_event.sl().file_name() != nullptr )
	{
		//	just write the basename
		std::string_view file{ i_event.sl().file_name() };
		auto pos = file.find_last_of( UPLATFORM_WIN ? '\\' : '/' );
		if ( pos != std::string_view::npos )
			file = file.substr( pos + 1 );
		ostr << "[" << file;
		if ( i_event.sl().function_name() != nullptr )
			ostr << ":" << i_event.sl().function_name();
		if ( i_event.sl().line() != -1 )
			ostr << ":" << i_event.sl().line();
		ostr << "]";
	}
	else if ( i_event.sl().function_name() != nullptr )
		ostr << "[" << i_event.sl().function_name() << "]";
	
	ostr << " ";
	
	i_event.message( ostr );
	ostr.write( "\n", 1 );
}

class logger_thread_data
{
private:
	int refCount = 1; // not protected, but typically only one logger_thread should be created
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
	
	void push( su::logger_base *i_logger, const std::chrono::system_clock::time_point &i_timestamp, std::thread::native_handle_type i_thread, su::log_event &&i_event );
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

void logger_thread_data::push( su::logger_base *i_logger,
								const std::chrono::system_clock::time_point &i_timestamp,
								std::thread::native_handle_type i_thread,
								su::log_event &&i_event )
{
	std::unique_lock<su::spinlock> l( queueSpinLock );
	logQueue.emplace_back( RecordedEvent{ i_logger, std::move(i_event), i_timestamp, i_thread } );
	l.unlock();
	queueCond.notify_one();
}

void logger_thread_data::flush()
{
	if ( not logQueueIsEmpty() )
	{
		std::unique_lock<std::mutex> l( queueMutex );
		queueCond.notify_one();
		queueCond.wait( l, [this](){ return this->logQueueIsEmpty(); } );
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
		queueCond.wait( l, [this](){ return not this->logQueueIsEmpty(); } );
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
				dump( rec.logger, rec.timestamp, rec.threadId, rec.event );
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
	: _level( i_level )
{
}

log_event::log_event( int i_level, su::source_location &&i_sl )
	: _level( i_level ),
		_sl( std::move(i_sl) )
{
}

log_event::~log_event()
{
	if ( not storageIsInline() )
		delete [] _storage.heapBuffer.data;
}

log_event::log_event( log_event &&lhs )
{
	_storage = lhs._storage;
	_buffer = lhs._buffer;
	_ptr = lhs._ptr;
	_level = lhs._level;
	_sl = lhs._sl;
	
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
		_level = lhs._level;
		_sl = lhs._sl;
		
		lhs._buffer = lhs._storage.inlineBuffer;
		lhs._ptr = lhs._buffer;
	}
	return *this;
}

void log_event::ensure_extra_capacity( size_t extra )
{
	auto currentSize = _ptr - _buffer;
	auto newSize = currentSize + extra;
	size_t cap = storageIsInline() ? kInlineBufferSize : _storage.heapBuffer.capacity;
	if ( newSize > cap )
	{
		auto newCap = (std::max)( newSize, cap * 2 );
		auto newBuffer = new char[newCap];
		memcpy( newBuffer, _buffer, currentSize );
		if ( not storageIsInline() )
			delete [] _storage.heapBuffer.data;
		
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
	*reinterpret_cast<log_data_type*>(_ptr) = TypeToEnum<T>::value;
	++_ptr;
	*reinterpret_cast<T*>(_ptr) = v;
	_ptr += sizeof( v );
}

log_event &log_event::operator<<( bool v ) { encode( v ); return *this; }
log_event &log_event::operator<<( char v ) { encode( v ); return *this; }
log_event &log_event::operator<<( unsigned char v ) { encode( v ); return *this; }
log_event &log_event::operator<<( short v ) { encode( v ); return *this; }
log_event &log_event::operator<<( unsigned short v ) { encode( v ); return *this; }
log_event &log_event::operator<<( int v ) { encode( v ); return *this; }
log_event &log_event::operator<<( unsigned int v ) { encode( v ); return *this; }
log_event &log_event::operator<<( long v ) { encode( v ); return *this; }
log_event &log_event::operator<<( unsigned long v ) { encode( v ); return *this; }
log_event &log_event::operator<<( long long v ) { encode( v ); return *this; }
log_event &log_event::operator<<( unsigned long long v ) { encode( v ); return *this; }
log_event &log_event::operator<<( double v ) { encode( v ); return *this; }

void log_event::encode_string_data( const char *i_data, size_t s )
{
	ensure_extra_capacity( sizeof( log_data_type ) + s + 1 );
	*reinterpret_cast<log_data_type*>(_ptr) = log_data_type::kStringData;
	_ptr += sizeof( log_data_type );
	memcpy( _ptr, i_data, s + 1 );
	_ptr += s + 1;
}

void log_event::encode_string_literal( const char *i_data )
{
	ensure_extra_capacity( sizeof( log_data_type ) + sizeof( const char * ) );
	*reinterpret_cast<log_data_type*>(_ptr) = log_data_type::kStringLiteral;
	_ptr += sizeof( log_data_type );
	*reinterpret_cast<const char **>(_ptr) = i_data;
	_ptr += sizeof( const char * );
}

std::string log_event::message() const
{
	std::ostringstream ostr;
	message( ostr );
	return ostr.str();
}

std::ostream &log_event::message( std::ostream &ostr ) const
{
	auto ptr = _buffer;
	auto end = _ptr;
	while ( ptr < end )
	{
		auto t = *reinterpret_cast<const log_data_type*>(ptr);
		ptr += sizeof( log_data_type );
		switch ( t )
		{
			case log_data_type::kBool:
				if ( *reinterpret_cast<const bool *>(ptr) )
					ostr << "true";
				else
					ostr << "false";
				ptr += sizeof( bool );
				break;
			case log_data_type::kChar:
				ostr << *reinterpret_cast<const char *>(ptr);
				ptr += sizeof( char );
				break;
			case log_data_type::kUnsignedChar:
				ostr << (int)*reinterpret_cast<const unsigned char *>(ptr);
				ptr += sizeof( unsigned char );
				break;
			case log_data_type::kShort:
				ostr << *reinterpret_cast<const short *>(ptr);
				ptr += sizeof( short );
				break;
			case log_data_type::kUnsignedShort:
				ostr << *reinterpret_cast<const unsigned short *>(ptr);
				ptr += sizeof( unsigned short );
				break;
			case log_data_type::kInt:
				ostr << *reinterpret_cast<const int *>(ptr);
				ptr += sizeof( int );
				break;
			case log_data_type::kUnsignedInt:
				ostr << *reinterpret_cast<const unsigned int *>(ptr);
				ptr += sizeof( unsigned int );
				break;
			case log_data_type::kLong:
				ostr << *reinterpret_cast<const long *>(ptr);
				ptr += sizeof( long );
				break;
			case log_data_type::kUnsignedLong:
				ostr << *reinterpret_cast<const unsigned long *>(ptr);
				ptr += sizeof( unsigned long );
				break;
			case log_data_type::kLongLong:
				ostr << *reinterpret_cast<const long long *>(ptr);
				ptr += sizeof( long long );
				break;
			case log_data_type::kUnsignedLongLong:
				ostr << *reinterpret_cast<const unsigned long long *>(ptr);
				ptr += sizeof( unsigned long long );
				break;
			case log_data_type::kDouble:
				ostr << *reinterpret_cast<const double *>(ptr);
				ptr += sizeof( double );
				break;
			case log_data_type::kStringLiteral:
				ostr << *reinterpret_cast<const char * const *>(ptr);
				ptr += sizeof( const char * );
				break;
			case log_data_type::kStringData:
				ostr << reinterpret_cast<const char *>(ptr);
				ptr += strlen( reinterpret_cast<const char *>(ptr) ) + 1;
				break;
			default:
				assert( false );
				break;
		}
	}
	return ostr;
}

Logger<kCOMPILETIME_LOG_MASK> logger( std::clog );

void logger_output::flush()
{
	ostr.flush();
}

logger_base::logger_base( std::unique_ptr<logger_output> &&i_output, const std::string_view &i_subsystem )
	: _output( std::move(i_output) ),
		_subsystem( i_subsystem )
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
#if UPLATFORM_WIN
	auto t = GetCurrentThread();
#else
	auto t = pthread_self();
#endif

	if ( g_thread != nullptr )
		g_thread->push( this, std::chrono::system_clock::now(), t, std::move(i_event) );
	else
		dump( this, std::chrono::system_clock::now(), t, i_event );
	return false;
}

std::unique_ptr<logger_output> logger_base::exchangeOutput( std::unique_ptr<logger_output> &&i_output )
{
	if ( g_thread != nullptr )
		g_thread->flush();
	else if ( _output )
		_output->flush();
	return std::exchange( _output, std::move(i_output) );
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