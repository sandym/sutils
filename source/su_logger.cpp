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
#include "su_flat_map.h"
#include "su_thread.h"
#include <ciso646>
#include <iostream>
#include <ctime>
#include <condition_variable>
#include <cassert>
#include <string.h>

namespace
{

enum class LogEventType : uint8_t
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
template<> struct TypeToEnum<bool>{ static const LogEventType value = LogEventType::kBool; };
template<> struct TypeToEnum<char>{ static const LogEventType value = LogEventType::kChar; };
template<> struct TypeToEnum<unsigned char>{ static const LogEventType value = LogEventType::kUnsignedChar; };
template<> struct TypeToEnum<short>{ static const LogEventType value = LogEventType::kShort; };
template<> struct TypeToEnum<unsigned short>{ static const LogEventType value = LogEventType::kUnsignedShort; };
template<> struct TypeToEnum<int>{ static const LogEventType value = LogEventType::kInt; };
template<> struct TypeToEnum<unsigned int>{ static const LogEventType value = LogEventType::kUnsignedInt; };
template<> struct TypeToEnum<long>{ static const LogEventType value = LogEventType::kLong; };
template<> struct TypeToEnum<unsigned long>{ static const LogEventType value = LogEventType::kUnsignedLong; };
template<> struct TypeToEnum<long long>{ static const LogEventType value = LogEventType::kLongLong; };
template<> struct TypeToEnum<unsigned long long>{ static const LogEventType value = LogEventType::kUnsignedLongLong; };
template<> struct TypeToEnum<double>{ static const LogEventType value = LogEventType::kDouble; };

su::spinlock g_loggerQueueLock;
std::condition_variable_any g_loggerQueueEvent;

std::vector<su::LoggerBase *> g_loggers;
std::thread g_loggerThread;

inline uint64_t timestamp()
{
	return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

class logger_thread_data
{
  private:
	int refCount = 1;
	std::mutex queueMutex;
	std::condition_variable queueCond;
	su::spinlock queueSpinLock;
	std::thread t;
	su::flat_map<su::LoggerBase *,std::vector<su::LogEvent>> logQueues;
	
	bool logQueuesAreEmpty();
	
	void func();
	
  public:
	logger_thread_data();
	
	inline void inc() { ++refCount; }
	void dec();
	
	void push( su::LoggerBase *i_logger, su::LogEvent &&i_event );
	void flush( su::LoggerBase *i_toRemove = nullptr );
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
		logQueues[nullptr].push_back( su::LogEvent( -1, {} ) );
		l.unlock();
		queueCond.notify_one();
		
		// and wait
		t.join();
		delete g_thread;
		g_thread = nullptr;
	}
}

bool logger_thread_data::logQueuesAreEmpty()
{
	std::unique_lock<su::spinlock> l( queueSpinLock );
	for ( auto &it : logQueues )
	{
		if ( not it.second.empty() )
			return false;
	}
	return true;
}

void logger_thread_data::push( su::LoggerBase *i_logger, su::LogEvent &&i_event )
{
	std::unique_lock<su::spinlock> l( queueSpinLock );
	logQueues[i_logger].push_back( std::move(i_event) );
	l.unlock();
	queueCond.notify_one();
}

void logger_thread_data::flush( su::LoggerBase *i_toRemove )
{
	std::unique_lock<std::mutex> l( queueMutex );
	if ( not logQueuesAreEmpty() )
	{
		queueCond.notify_one();
		while ( not logQueuesAreEmpty() )
			queueCond.wait( l );
	}
	l.unlock();

	std::unique_lock<su::spinlock> sl( queueSpinLock );
	auto it = logQueues.find( i_toRemove );
	if ( it != logQueues.end() )
		logQueues.erase( it );
}

void logger_thread_data::func()
{
	std::unique_lock<std::mutex> l( queueMutex );
	su::flat_map<su::LoggerBase *,std::vector<su::LogEvent>> localCopy;
	bool exitLoop = false;
	while ( not exitLoop )
	{
		// wait for logs
		while ( logQueuesAreEmpty() )
			queueCond.wait( l );
		l.unlock();
		
		// quickly grab a local copy
		std::unique_lock<su::spinlock> sl( queueSpinLock );
		localCopy.swap( logQueues );
		sl.unlock();
		
		for ( auto &ll : localCopy )
		{
			if ( ll.first == nullptr )
				exitLoop = true;
			else
			{
				for ( auto &e : ll.second )
					e.dump( ll.first->subsystem(), ll.first->ostr() );
				ll.first->ostr().flush();
			}
		}
		
		localCopy.clear();
		
		// notify
		queueCond.notify_all();
		
		l.lock();
	}
}

}

namespace su
{

LogEvent::LogEvent( int i_level )
	: _level( i_level ),
		_threadID( std::this_thread::get_id() ),
		_time( timestamp() )
{
}
LogEvent::LogEvent( int i_level, su::source_location &&i_sl )
	: _level( i_level ),
		_sl( std::move(i_sl) ),
		_threadID( std::this_thread::get_id() ),
		_time( timestamp() )
{
}

void LogEvent::ensure_extra_capacity( size_t extra )
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
void LogEvent::encode( const T &v )
{
	ensure_extra_capacity( 1 + sizeof( v ) );
	*reinterpret_cast<LogEventType*>(_ptr) = TypeToEnum<T>::value;
	++_ptr;
	*reinterpret_cast<T*>(_ptr) = v;
	_ptr += sizeof( v );
}

LogEvent &LogEvent::operator<<( bool v ) { encode( v ); return *this; }
LogEvent &LogEvent::operator<<( char v ) { encode( v ); return *this; }
LogEvent &LogEvent::operator<<( unsigned char v ) { encode( v ); return *this; }
LogEvent &LogEvent::operator<<( short v ) { encode( v ); return *this; }
LogEvent &LogEvent::operator<<( unsigned short v ) { encode( v ); return *this; }
LogEvent &LogEvent::operator<<( int v ) { encode( v ); return *this; }
LogEvent &LogEvent::operator<<( unsigned int v ) { encode( v ); return *this; }
LogEvent &LogEvent::operator<<( long v ) { encode( v ); return *this; }
LogEvent &LogEvent::operator<<( unsigned long v ) { encode( v ); return *this; }
LogEvent &LogEvent::operator<<( long long v ) { encode( v ); return *this; }
LogEvent &LogEvent::operator<<( unsigned long long v ) { encode( v ); return *this; }
LogEvent &LogEvent::operator<<( double v ) { encode( v ); return *this; }

void LogEvent::encode_string_data( const char *i_data, size_t s )
{
	ensure_extra_capacity( sizeof( LogEventType ) + s + 1 );
	*reinterpret_cast<LogEventType*>(_ptr) = LogEventType::kStringData;
	_ptr += sizeof( LogEventType );
	memcpy( _ptr, i_data, s + 1 );
	_ptr += s + 1;
}

void LogEvent::encode_string_literal( const char *i_data )
{
	ensure_extra_capacity( sizeof( LogEventType ) + sizeof( const char * ) );
	*reinterpret_cast<LogEventType*>(_ptr) = LogEventType::kStringLiteral;
	_ptr += sizeof( LogEventType );
	*reinterpret_cast<const char **>(_ptr) = i_data;
	_ptr += sizeof( const char * );
}

void LogEvent::dump( const std::string &i_ss, std::ostream &ostr ) const
{
	// [TIME][LEVEL][thread][subsystem][file:func:line] msg
	
	const char *level = "";
	switch ( _level )
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

	std::time_t t = _time / 1000000;
	char isoTime[32] = "";
	char ms[8] = "";
	struct tm gmtm;
#if UPLATFORM_WIN
	if (gmtime_s(&gmtm, &t) == 0)
	{
		strftime(isoTime, 32, "%Y-%m-%dT%T", &gmtm);
		sprintf_s(ms, ".%06lu", (unsigned long)(_time % 1000000));
	}
#else
	strftime( isoTime, 32, "%Y-%m-%dT%T", gmtime_r( &t, &gmtm ) );
	sprintf(ms, ".%06lu", (unsigned long)(_time % 1000000));
#endif
	
	ostr << "[" << isoTime << ms << "][" << level << "][" << _threadID << "]";
	if ( not i_ss.empty() )
		ostr << "[" << i_ss << "]";

	// output location, if available
	if ( _sl.file_name() != nullptr )
	{
		//	remove the path base, just write the filename
		const char *file = _sl.file_name();
		std::reverse_iterator<const char *> begin( file + strlen( file ) );
		std::reverse_iterator<const char *> end( file );
		std::reverse_iterator<const char *> pos = std::find( begin, end, UPLATFORM_WIN ? '\\' : '/' );
		if ( pos != end )
			file = pos.base();
		ostr << "[" << file;
		if ( _sl.function_name() != nullptr )
			ostr << ":" << _sl.function_name();
		ostr << ":" << _sl.line() << "] ";
	}
	else if ( _sl.function_name() != nullptr )
		ostr << "[" << _sl.function_name() << "] ";
	else
		ostr << " ";
	
	auto ptr = _buffer;
	auto end = _ptr;
	while ( ptr < end )
	{
		auto t = *reinterpret_cast<const LogEventType*>(ptr);
		ptr += sizeof( LogEventType );
		switch ( t )
		{
			case LogEventType::kBool:
				if ( *reinterpret_cast<const bool *>(ptr) )
					ostr << "true";
				else
					ostr << "false";
				ptr += sizeof( bool );
				break;
			case LogEventType::kChar:
				ostr << *reinterpret_cast<const char *>(ptr);
				ptr += sizeof( char );
				break;
			case LogEventType::kUnsignedChar:
				ostr << *reinterpret_cast<const unsigned char *>(ptr);
				ptr += sizeof( unsigned char );
				break;
			case LogEventType::kShort:
				ostr << *reinterpret_cast<const short *>(ptr);
				ptr += sizeof( short );
				break;
			case LogEventType::kUnsignedShort:
				ostr << *reinterpret_cast<const unsigned short *>(ptr);
				ptr += sizeof( unsigned short );
				break;
			case LogEventType::kInt:
				ostr << *reinterpret_cast<const int *>(ptr);
				ptr += sizeof( int );
				break;
			case LogEventType::kUnsignedInt:
				ostr << *reinterpret_cast<const unsigned int *>(ptr);
				ptr += sizeof( unsigned int );
				break;
			case LogEventType::kLong:
				ostr << *reinterpret_cast<const long *>(ptr);
				ptr += sizeof( long );
				break;
			case LogEventType::kUnsignedLong:
				ostr << *reinterpret_cast<const unsigned long *>(ptr);
				ptr += sizeof( unsigned long );
				break;
			case LogEventType::kLongLong:
				ostr << *reinterpret_cast<const long long *>(ptr);
				ptr += sizeof( long long );
				break;
			case LogEventType::kUnsignedLongLong:
				ostr << *reinterpret_cast<const unsigned long long *>(ptr);
				ptr += sizeof( unsigned long long );
				break;
			case LogEventType::kDouble:
				ostr << *reinterpret_cast<const double *>(ptr);
				ptr += sizeof( double );
				break;
			case LogEventType::kStringLiteral:
				ostr << *reinterpret_cast<const char * const  *>(ptr);
				ptr += sizeof( const char * );
				break;
			case LogEventType::kStringData:
				ostr << reinterpret_cast<const char *>(ptr);
				ptr += strlen( reinterpret_cast<const char *>(ptr) ) + 1;
				break;
			default:
				assert( false );
				break;
		}
	}
	ostr.write( "\n", 1 );
}

Logger<kCOMPILETIME_LOG_MASK> logger( std::clog );

LoggerBase::LoggerBase( std::ostream &ostr, const std::string &i_subsystem )
	: _ostr( ostr ),
		_subsystem( i_subsystem )
{
}

LoggerBase::~LoggerBase()
{
	if ( g_thread != nullptr )
	{
		g_thread->flush( this );
	}
	else
	{
		_ostr.flush();
	}
}

bool LoggerBase::operator==( LogEvent &i_event )
{
	if ( g_thread != nullptr )
		g_thread->push( this, std::move(i_event) );
	else
		i_event.dump( _subsystem, _ostr );
	return false;
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
