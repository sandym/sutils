
#include "su_json_flat.h"
#include "su_endian.h"
#include "su_logger.h"
#include "su_stackarray.h"
#include <iostream>
#include <cassert>

namespace {

enum
{
	kVersion1 = 81,
	
	kLatestVersion = kVersion1
};

// Lookup tables for faster decoding
const uint8_t trailingBytes[256] = {
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, 2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
	3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3, 4,4,4,4,4,4,4,4,5,5,5,5,6,6,7,8
};
const uint8_t headerMask[256] = {
	0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
	0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
	0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
	0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
	0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
	0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
	0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
	0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
	0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,
	0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,
	0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,
	0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,
	0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,
	0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,
	0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,
	0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x07,0x07,0x07,0x07,0x03,0x03,0x01,0x00
};

// helper class for serializing/deserializing integer types with maximum compression
// it is then specialzed for 1, 2, 4 and 8 bytes integer type.  Should be 64 bit safe.
template<typename T,size_t S>
struct helper_int
{
	static void flatten( std::ostream &ostr, T v );
	static T unflatten( std::istream &istr );
};


template<typename T>
struct helper_int<T,1>
{
	static void flatten( std::ostream &ostr, T v )
	{
		ostr.write( (const char *)&v, 1 );
	}
	static T unflatten( std::istream &istr )
	{
		T v = 0;
		istr.read( (char *)&v, 1 );
		return v;
	}
};
template<typename T>
struct helper_int<T,4>
{
	static void flatten( std::ostream &ostr, T v )
	{
		uint32_t vv = static_cast<uint32_t>( v );
		uint8_t b[5], *dest, marker;
		int bytesToWrite;
		if ( vv <= 0x7F )			{ bytesToWrite = 1; marker = 0x00; }
		else if ( vv <= 0x3FFF )	{ bytesToWrite = 2; marker = 0x80; }
		else if ( vv <= 0x1FFFFF )	{ bytesToWrite = 3; marker = 0xC0; }
		else if ( vv <= 0xFFFFFFF )	{ bytesToWrite = 4; marker = 0xE0; }
		else						{ bytesToWrite = 5; marker = 0xF0; }
	
		dest = b + bytesToWrite;
		switch ( bytesToWrite )
		{
			case 5:	*--dest = vv&0xFF; vv >>= 8;
			case 4:	*--dest = vv&0xFF; vv >>= 8;
			case 3:	*--dest = vv&0xFF; vv >>= 8;
			case 2:	*--dest = vv&0xFF; vv >>= 8;
			case 1:	*--dest = (marker | (vv&0xFF));
		}
		ostr.write( (const char *)b, bytesToWrite );
	}
	static T unflatten( std::istream &istr )
	{
		uint8_t b[5], *src = b;
		istr.read( (char *)b, 1 );
		uint8_t extraBytes = trailingBytes[b[0]];
		assert( extraBytes <= 4 );
		if ( extraBytes > 0 )
			istr.read( (char *)b+1, extraBytes );
		uint32_t vv = (*src++)&headerMask[b[0]];
		switch ( extraBytes )
		{
			case 4: vv <<= 8; vv |= *src++;
			case 3: vv <<= 8; vv |= *src++;
			case 2: vv <<= 8; vv |= *src++;
			case 1: vv <<= 8; vv |= *src++;
		}
		return static_cast<T>( vv );
	}
};
template<typename T>
struct helper_int<T,8>
{
	static void flatten( std::ostream &ostr, T v )
	{
		uint64_t vv = static_cast<uint64_t>( v );
		uint8_t b[9], *dest, marker;
		int bytesToWrite;
		if ( vv <= 0x7F )					{ bytesToWrite = 1; marker = 0x00; }
		else if ( vv <= 0x3FFFLL )			{ bytesToWrite = 2; marker = 0x80; }
		else if ( vv <= 0x1FFFFFLL )		{ bytesToWrite = 3; marker = 0xC0; }
		else if ( vv <= 0xFFFFFFFLL )		{ bytesToWrite = 4; marker = 0xE0; }
		else if ( vv <= 0x7FFFFFFFFLL )		{ bytesToWrite = 5; marker = 0xF0; }
		else if ( vv <= 0x3FFFFFFFFFFLL )	{ bytesToWrite = 6; marker = 0xF8; }
		else if ( vv <= 0x1FFFFFFFFFFFFLL )	{ bytesToWrite = 7; marker = 0xFC; }
		else if ( vv <= 0xFFFFFFFFFFFFFFLL ){ bytesToWrite = 8; marker = 0xFE; }
		else								{ bytesToWrite = 9; marker = 0xFF; }
		
		dest = b + bytesToWrite;
		switch ( bytesToWrite )
		{
			case 9:	*--dest = uint8_t(vv&0xFF); vv >>= 8;
			case 8:	*--dest = uint8_t(vv&0xFF); vv >>= 8;
			case 7:	*--dest = uint8_t(vv&0xFF); vv >>= 8;
			case 6:	*--dest = uint8_t(vv&0xFF); vv >>= 8;
			case 5:	*--dest = uint8_t(vv&0xFF); vv >>= 8;
			case 4:	*--dest = uint8_t(vv&0xFF); vv >>= 8;
			case 3:	*--dest = uint8_t(vv&0xFF); vv >>= 8;
			case 2:	*--dest = uint8_t(vv&0xFF); vv >>= 8;
			case 1:	*--dest = uint8_t(marker | (vv&0xFF));
		}
		ostr.write( (const char *)b, bytesToWrite );
	}
	static T unflatten( std::istream &istr )
	{
		uint8_t b[9], *src = b;
		istr.read( (char *)b, 1 );
		uint8_t extraBytes = trailingBytes[b[0]];
		assert( extraBytes <= 8 );
		if ( extraBytes > 0 )
			istr.read( (char *)b+1, extraBytes );
		uint64_t vv = (*src++)&headerMask[b[0]];
		switch ( extraBytes )
		{
			case 8: vv <<= 8; vv |= *src++;
			case 7: vv <<= 8; vv |= *src++;
			case 6: vv <<= 8; vv |= *src++;
			case 5: vv <<= 8; vv |= *src++;
			case 4: vv <<= 8; vv |= *src++;
			case 3: vv <<= 8; vv |= *src++;
			case 2: vv <<= 8; vv |= *src++;
			case 1: vv <<= 8; vv |= *src++;
		}
		return static_cast<T>( vv );
	}
};

class flattener
{
public:
	flattener( std::ostream &ostr ) : _ostr( ostr ){}
	void operator()( const su::Json &i_json );
	
private:
	std::ostream &_ostr;
	su::flat_map<std::string,size_t> _stringDict;

	template<typename T>
	void flatten_value( const T &v );

	void flatten( const su::Json &i_json );
	void flatten_no_type( const su::Json &i_json );

	void writeString( const std::string &i_string );
	void CollectString( const std::string &i_string );
	void CollectStrings( const su::Json &i_json );
};

template<>
void flattener::flatten_value<bool>( const bool &v )
{
	uint8_t b = v ? 1 : 0;
	helper_int<uint8_t,sizeof(uint8_t)>::flatten( _ostr, b );
}
template<>
void flattener::flatten_value<double>( const double &v )
{
	double n = su::native_to_little<double>( v );
	_ostr.write( (const char *)&n, sizeof( double ) );
}
template<>
void flattener::flatten_value<std::string>( const std::string &v )
{
	flatten_value<size_t>( v.length() );
	_ostr.write( v.data(), v.length() );
}
template<typename T>
void flattener::flatten_value( const T &v )
{
	helper_int<T,sizeof(T)>::flatten( _ostr, v );
}

void flattener::operator()( const su::Json &i_json )
{
	// save the version
	flatten_value<uint32_t>( kLatestVersion );
	
	// collect the string dictionary
	CollectStrings( i_json );
	
	// save the string dictionary
	flatten_value<size_t>( _stringDict.size() );
	size_t index = 0;
	for ( auto &it : _stringDict )
	{
		it.second = ++index;
		flatten_value<std::string>( it.first );
	}
	
	// flatten using the dictionary
	flatten( i_json );
}

enum encoding_t
{
	encoding_nul,
	encoding_bool,
	encoding_string,
	encoding_double,
	encoding_int,
	encoding_int64,
	encoding_array,
	encoding_object
};

uint8_t encoding_type( const su::Json &i_json )
{
	static_assert( encoding_int64 <= 0x07, "" ); // has to be on 3 bits
	switch ( i_json.type() )
	{
		case su::Json::Type::NUL: return encoding_nul;
		case su::Json::Type::BOOL: return encoding_bool;
		case su::Json::Type::STRING: return encoding_string;
		case su::Json::Type::NUMBER:
			switch ( i_json.number_type() )
			{
				case su::Json::NumberType::DOUBLE: return encoding_double;
				case su::Json::NumberType::INTEGER: return encoding_int;
				case su::Json::NumberType::INTEGER64: return encoding_int64;
				default:
					assert( false );
					break;
			}
		case su::Json::Type::ARRAY: return encoding_array;
		case su::Json::Type::OBJECT: return encoding_object;
		default:
			assert( false );
			break;
	}
	return 0;
}
su::optional<uint8_t> homogeneousType( const su::Json::array &i_array )
{
	assert( not i_array.empty() );
	auto type = encoding_type( i_array.front() );
	
	// reject arrays
	if ( type == encoding_array )
		return {};
	
	for ( auto &it : i_array )
	{
		if ( type != encoding_type( it ) )
			return {};
	}
	return type;
}

void flattener::flatten( const su::Json &i_json )
{
	uint8_t type = encoding_type( i_json);
	/* type:
		type&0x7, the first 3 bits are always the type of the entry
			BOOL:
				type&0x80, the last bit is the value
			NUMBER:
	 			type&0x80, the last bit is set if the value is 0 otherwise
				the value follow
			STRING:
	 			type&0x80, the last bit is set if the string is empty otherwise
				the value follow
			NUL:
				nothing else encoded
			ARRAY:
				type&0x80, the last bit is set if the array is empty otherwise
	 			type&0x40 is set if the array is homogeneous (all the same type) and
	 			if homogeneous:
					type&0x38 is the type of all element in the array
					length and
					the array values follow with no types
	 			else
					length and the array values follow
			OBJECT:
				type&0x80, the last bit is set if the object is empty otherwise
				length and
				the object key-value pairs follow
	*/
	
	switch ( type )
	{
		case encoding_nul:
			flatten_value<uint8_t>( type );
			break;
		case encoding_bool:
			// all in one byte
			if ( i_json.bool_value() )
				type |= 0x80;
			flatten_value<uint8_t>( type );
			break;
		case encoding_string:
		{
			auto &s = i_json.string_value();
			if ( s.empty() )
			{
				// empty string in one byte
				type |= 0x80;
				flatten_value<uint8_t>( type );
			}
			else
			{
				flatten_value<uint8_t>( type );
				writeString( s );
			}
			break;
		}
		case encoding_double:
			if ( i_json.number_value() == 0.0 )
			{
				type |= 0x80;
				flatten_value<uint8_t>( type );
			}
			else
			{
				flatten_value<uint8_t>( type );
				flatten_value<double>( i_json.number_value() );
			}
			break;
		case encoding_int:
			if ( i_json.int_value() == 0 )
			{
				type |= 0x80;
				flatten_value<uint8_t>( type );
			}
			else
			{
				flatten_value<uint8_t>( type );
				flatten_value<int>( i_json.int_value() );
			}
			break;
		case encoding_int64:
			if ( i_json.int64_value() == 0 )
			{
				type |= 0x80;
				flatten_value<uint8_t>( type );
			}
			else
			{
				flatten_value<uint8_t>( type );
				flatten_value<int>( i_json.int64_value() );
			}
			break;
		case encoding_array:
		{
			auto &a = i_json.array_items();
			if ( a.empty() )
			{
				// empty array as one byte
				type |= 0x80;
				flatten_value<uint8_t>( type );
			}
			else
			{
				auto hType = homogeneousType( a );
				if ( hType )
				{
					type |= 0x40;
					type |= (*hType<<3);
					flatten_value<uint8_t>( type );
					flatten_value<size_t>( a.size() );
					for ( auto &it : a )
						flatten_no_type( it );
				}
				else
				{
					flatten_value<uint8_t>( type );
					flatten_value<size_t>( a.size() );
					for ( auto &it : a )
						flatten( it );
				}
			}
			break;
		}
		case encoding_object:
		{
			auto &o = i_json.object_items();
			if ( o.empty() )
			{
				type |= 0x80;
				flatten_value<uint8_t>( type );
			}
			else
			{
				flatten_value<uint8_t>( type );
				flatten_value<size_t>( o.size() );
				for ( auto &it : o )
				{
					writeString( it.first );
					flatten( it.second );
				}
			}
			break;
		}
		default:
			assert( false );
			break;
	}
}

void flattener::flatten_no_type( const su::Json &i_json )
{
	uint8_t type = encoding_type( i_json);
	switch ( type )
	{
		case encoding_nul:
			break;
		case encoding_bool:
			// all in one byte
			if ( i_json.bool_value() )
				type |= 0x80;
			flatten_value<uint8_t>( type );
			break;
		case encoding_string:
			writeString( i_json.string_value() );
			break;
		case encoding_double:
			flatten_value<double>( i_json.number_value() );
			break;
		case encoding_int:
			flatten_value<int>( i_json.int_value() );
			break;
		case encoding_int64:
			flatten_value<int>( i_json.int64_value() );
			break;
		case encoding_array:
			assert( false );
			break;
		case encoding_object:
		{
			auto &o = i_json.object_items();
			flatten_value<size_t>( o.size() );
			for ( auto &it : o )
			{
				writeString( it.first );
				flatten( it.second );
			}
			break;
		}
		default:
			assert( false );
			break;
	}
}

void flattener::writeString( const std::string &i_string )
{
	if ( i_string.empty() )
		flatten_value<size_t>( 0 );
	else
	{
		auto it = _stringDict.find( i_string );
		assert( it != _stringDict.end() );
		flatten_value<size_t>( it->second );
	}
}

void flattener::CollectString( const std::string &i_string )
{
	if ( _stringDict.find( i_string ) == _stringDict.end() )
		_stringDict[i_string] = _stringDict.size() + 1;
}

void flattener::CollectStrings( const su::Json &i_json )
{
	switch ( i_json.type() )
	{
		case su::Json::Type::STRING:
			CollectString( i_json.string_value() );
			break;
		case su::Json::Type::ARRAY:
			for ( auto &it : i_json.array_items() )
				CollectStrings( it );
			break;
		case su::Json::Type::OBJECT:
			for ( auto &it : i_json.object_items() )
			{
				CollectString( it.first );
				CollectStrings( it.second );
			}
			break;
		default:
			break;
	}
}

}

namespace su {

void flatten( const Json &i_json, std::ostream &ostr )
{
	flattener f( ostr );
	f( i_json );
}

}

namespace {

class unflattener
{
public:
	unflattener( std::istream &istr ) : _istr( istr ){}
	
	su::Json operator()();
	
private:
	std::istream &_istr;
	uint32_t _version = 0;
	std::vector<std::string> _stringDict;
	
	su::Json unflatten();
	su::Json unflatten_type( uint8_t type );
	
	template<typename T>
	T unflatten_value();

	const std::string &readString();

	const static size_t kSaneValueForNbOfEntry = 65536*2;
};

template<>
bool unflattener::unflatten_value<bool>()
{
	return (helper_int<uint8_t,sizeof(uint8_t)>::unflatten( _istr ) != 0);
}
template<>
double unflattener::unflatten_value<double>()
{
	double v = 0;
	_istr.read( (char *)&v, sizeof( double ) );
	return su::little_to_native<double>( v );
}
template<>
std::string unflattener::unflatten_value<std::string>()
{
	size_t len = unflatten_value<size_t>();
	if ( len > kSaneValueForNbOfEntry )
		throw std::out_of_range( "string too big in flat json" );
	su::stackarray<char> buffer( len );
	_istr.read( buffer.data(), len );
	return std::string( buffer.data(), len );
}
template<typename T>
T unflattener::unflatten_value()
{
	return helper_int<T,sizeof(T)>::unflatten( _istr );
}

inline const std::string &unflattener::readString()
{
	auto i = unflatten_value<size_t>() - 1;
	if ( i >= _stringDict.size() )
		throw std::runtime_error( "string not found in flat json dictionary" );
	return _stringDict[i];
}

su::Json unflattener::operator()()
{
	// get the version
	_version = unflatten_value<uint32_t>();
	if ( _version < 1 or _version > kLatestVersion )
		throw std::runtime_error( "invalid flat json" );
	
	// read the string dictionary
	auto l = unflatten_value<size_t>();
	if ( l > kSaneValueForNbOfEntry )
		throw std::out_of_range( "dictionary too big in flat json" );
	_stringDict.emplace_back( "" );
	for ( size_t i = 0; i < l; ++i )
		_stringDict.push_back( unflatten_value<std::string>() );
	
	// unflatten the rest using the string dictionary
	return unflatten();
}

su::Json unflattener::unflatten()
{
	auto type = unflatten_value<uint8_t>();
	switch ( type )
	{
		case encoding_nul:
			return su::Json();
		case encoding_bool:
			return su::Json( (type&0x80) != 0 );
		case encoding_string:
			if ( (type&0x80) != 0 )
				return su::Json("");
			else
				return su::Json( readString() );
		case encoding_double:
			if ( (type&0x80) != 0 )
				return su::Json( double(0.0) );
			else
				return su::Json( unflatten_value<double>() );
		case encoding_int:
			if ( (type&0x80) != 0 )
				return su::Json( int(0) );
			else
				return su::Json( unflatten_value<int>() );
		case encoding_int64:
			if ( (type&0x80) != 0 )
				return su::Json( int64_t(0) );
			else
				return su::Json( unflatten_value<int64_t>() );
		case encoding_array:
			if ( (type&0x80) != 0 )
				return su::Json::array();
			else
			{
				su::Json::array a;
				auto l = unflatten_value<size_t>();
				if ( l > kSaneValueForNbOfEntry )
					throw std::out_of_range( "array too big in flat json" );
				if ( (type&0x40) != 0 )
				{
					// homogeneous
					uint8_t hType = (type&0x38)>>3;
					for ( size_t i = 0; i < l; ++i )
						a.push_back( unflatten_type( hType ) );
					return a;
				}
				else
				{
					// heterogeneous
					for ( size_t i = 0; i < l; ++i )
						a.push_back( unflatten() );
				}
				return a;
			}
		case encoding_object:
			if ( (type&0x80) != 0 )
				return su::Json::object();
			else
			{
				su::Json::object o;
				auto l = unflatten_value<size_t>();
				if ( l > kSaneValueForNbOfEntry )
					throw std::out_of_range( "object too big in flat json" );
				for ( size_t i = 0; i < l; ++i )
				{
					auto k = readString();
					o[k] = unflatten();
				}
				return o;
			}
		default:
			throw std::runtime_error( "invalid type in flat json" );
			break;
	}
	
	return {};
}

su::Json unflattener::unflatten_type( uint8_t type )
{
	switch ( type )
	{
		case encoding_nul:
			return {};
		case encoding_bool:
			return su::Json( (unflatten_value<uint8_t>()&0x80) != 0 );
		case encoding_string:
			return su::Json( readString() );
			break;
		case encoding_double:
			return su::Json( unflatten_value<double>() );
		case encoding_int:
			return su::Json( unflatten_value<int>() );
		case encoding_int64:
			return su::Json( unflatten_value<int64_t>() );
		case encoding_array:
			assert( false );
			break;
		case encoding_object:
		{
			su::Json::object o;
			auto l = unflatten_value<size_t>();
			if ( l > kSaneValueForNbOfEntry )
				throw std::out_of_range( "object too big in flat json" );
			for ( size_t i = 0; i < l; ++i )
			{
				auto k = readString();
				o[k] = unflatten();
			}
			return o;
		}
		default:
			assert( false );
			break;
	}

	return {};
}

}

namespace su {

su::optional<su::Json> unflatten( std::istream &istr )
{
	try
	{
		unflattener uf( istr );
		return uf();
	}
	catch ( std::exception &ex )
	{
		log_error() << ex.what();
	}
	return {};
}

}
