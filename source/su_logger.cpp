/*
 *  su_logger.cpp
 *  sutils
 *
 *  Created by Sandy Martel on 08-04-20.
 *  Copyright (c) 2015年 Sandy Martel. All rights reserved.
 *
 * Permission to use, copy, modify, distribute, and sell this software for any purpose is hereby
 * granted without fee. The sotware is provided "AS-IS" and without warranty of any kind, express,
 * implied or otherwise.
 */

#include "su_logger.h"
#include "su_spinlock.h"
#include "su_platform.h"
#include "su_flat_set.h"
#include "su_thread.h"
#include <iostream>
#include <ctime>
#include <condition_variable>
#include <cassert>
#include <sstream>
#include <string.h>

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

//! generate timestamp in microseconds
inline uint64_t get_timestamp()
{
	return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

class logger_thread_data
{
private:
	int refCount = 1; // not protected, but typically only one logger_thread should be created
	std::mutex queueMutex;
	std::condition_variable queueCond;
	su::spinlock queueSpinLock;
	std::thread t;
	std::vector<std::pair<su::logger_base *,su::log_event>> logQueue;
	
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
		logQueue.emplace_back( nullptr, su::log_event( -1, {} ) );
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

void logger_thread_data::push( su::logger_base *i_logger, su::log_event &&i_event )
{
	std::unique_lock<su::spinlock> l( queueSpinLock );
	logQueue.emplace_back( i_logger, std::move(i_event) );
	l.unlock();
	queueCond.notify_one();
}

void logger_thread_data::flush()
{
	if ( not logQueueIsEmpty() )
	{
		std::unique_lock<std::mutex> l( queueMutex );
		queueCond.notify_one();
		while ( not logQueueIsEmpty() )
			queueCond.wait( l );
	}
}

void logger_thread_data::func()
{
	su::this_thread::set_name( "logger_thread" );
	
	std::unique_lock<std::mutex> l( queueMutex, std::defer_lock );
	
	std::vector<std::pair<su::logger_base *,su::log_event>> localCopy;
	su::flat_set<su::logger_base *> toFlush;
	
	bool exitLoop = false;
	while ( not exitLoop )
	{
		l.lock();
		// wait for logs
		while ( logQueueIsEmpty() )
			queueCond.wait( l );
		l.unlock();
		
		// quickly grab a local copy
		std::unique_lock<su::spinlock> sl( queueSpinLock );
		localCopy.swap( logQueue );
		sl.unlock();
		
		// dump all events
		for ( auto &ev : localCopy )
		{
			if ( ev.first == nullptr )
				exitLoop = true;
			else if ( ev.first->output() )
			{
				ev.first->dump( ev.second );
				toFlush.insert( ev.first );
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


inline pthread_t current_thread() { return pthread_self(); }

struct thread_name
{
	pthread_t _t;
	thread_name( pthread_t t ) : _t( t ){}
};
std::ostream &operator<<( std::ostream &os, const thread_name &t )
{
	char buffer[20];
	if ( pthread_getname_np( t._t, buffer, 20 ) == 0 and buffer[0] != 0 )
		return os << buffer;
	else
		return os << t._t;
}

}

namespace su {

log_event::log_event( int i_level )
	: _level( i_level ),
		_threadID( current_thread() ),
		_timestamp( get_timestamp() )
{
}
log_event::log_event( int i_level, su::source_location &&i_sl )
	: _level( i_level ),
		_sl( std::move(i_sl) ),
		_threadID( current_thread() ),
		_timestamp( get_timestamp() )
{
}

void log_event::ensure_extra_capacity( size_t extra )
{
	auto currentSize = _ptr - _buffer;
	auto newSize = currentSize + extra;
	if ( newSize > _capacity )
	{
		auto newCap = std::max( newSize, _capacity * 2 );
		auto newBuffer = new char[newCap];
		memcpy( newBuffer, _buffer, currentSize );
		_capacity = newCap;
		_heapBuffer.reset( newBuffer );
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

logger_base::logger_base( std::unique_ptr<logger_output> &&i_output, const std::string &i_subsystem )
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
	if ( g_thread != nullptr )
		g_thread->push( this, std::move(i_event) );
	else
		dump( i_event );
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

void logger_base::dump( const su::log_event &i_event )
{
	if ( _output.get() == nullptr )
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

	std::time_t t = i_event.timestamp() / 1000000;
	char isoTime[32] = "";
	char ms[8] = "";
	struct tm tmdata;
#if UPLATFORM_WIN
	if ( localtime_s( &tmdata, &t ) == 0 )
	{
		strftime(isoTime, 32, "%Y-%m-%dT%T", &tmdata);
		sprintf_s(ms, ".%06lu", (unsigned long)(i_event.timestamp() % 1000000));
	}
#else
	strftime( isoTime, 32, "%Y-%m-%dT%T", localtime_r( &t, &tmdata ) );
	sprintf(ms, ".%06lu", (unsigned long)(i_event.timestamp() % 1000000));
#endif
	
	auto &ostr = _output->ostr;
	
	ostr << "[" << isoTime << ms << "][" << level << "][" << thread_name(i_event.threadID()) << "]";
	if ( not _subsystem.empty() )
		ostr << "[" << _subsystem << "]";

	// output location, if available
	if ( i_event.sl().file_name() != nullptr )
	{
		//	remove the path base, just write the filename
		const char *file = i_event.sl().file_name();
		std::reverse_iterator<const char *> begin( file + strlen( file ) );
		std::reverse_iterator<const char *> end( file );
		std::reverse_iterator<const char *> pos = std::find( begin, end, UPLATFORM_WIN ? '\\' : '/' );
		if ( pos != end )
			file = pos.base();
		ostr << "[" << file;
		if ( i_event.sl().function_name() != nullptr )
			ostr << ":" << i_event.sl().function_name();
		ostr << ":" << i_event.sl().line() << "] ";
	}
	else if ( i_event.sl().function_name() != nullptr )
		ostr << "[" << i_event.sl().function_name() << "] ";
	else
		ostr << " ";
	
	i_event.message( ostr );
	ostr.write( "\n", 1 );
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
