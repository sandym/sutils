#ifndef H_SU_JSON
#define H_SU_JSON

#include <string_view>
#include <string>
#include "su_flat_map.h"

namespace su {

enum class JsonParse
{
	STANDARD,
	COMMENTS
};

namespace details {
struct JsonValue;
}

class Json final
{
public:
	// Types
	enum class Type
	{
		NUL,
		BOOL,
		NUMBER,
		STRING,
		ARRAY,
		OBJECT
	};

	// Array and object typedefs
	typedef std::vector<Json> array;
	typedef flat_map<std::string, Json, std::less<>> object;

	~Json();

	Json( const Json &rhs ) noexcept;
	Json &operator=( const Json &rhs ) noexcept;
	Json( Json &&rhs ) noexcept;
	Json &operator=( Json &&rhs ) noexcept;

	// Constructors for the various types of JSON value.
	Json() noexcept {} // NUL
	Json( std::nullptr_t ) noexcept {} // NUL
	Json( double value ); // NUMBER
	Json( int value ); // NUMBER
	Json( unsigned int value ); // NUMBER
	Json( long value ); // NUMBER
	Json( unsigned long value ); // NUMBER
	Json( long long value ); // NUMBER
	Json( unsigned long long value ); // NUMBER
	Json( bool value ); // BOOL
	Json( const std::string &value ); // STRING
	Json( std::string &&value ); // STRING
	Json( const char *value ); // STRING
	Json( const array &values ); // ARRAY
	Json( array &&values ); // ARRAY
	Json( const object &values ); // OBJECT
	Json( object &&values ); // OBJECT

	// Implicit constructor: anything with a to_json() function.
	template<class T, class = decltype( &T::to_json )>
	Json( const T &t ) : Json( t.to_json() )
	{
	}

	// Implicit constructor: map-like objects (std::map, std::unordered_map,
	// etc)
	template<class M,
	         std::enable_if_t<
	             std::is_constructible_v<std::string, typename M::key_type> &&
	                 std::is_constructible_v<Json, typename M::mapped_type>,
	             int> = 0>
	Json( const M &m ) : Json( object( m.begin(), m.end() ) )
	{
	}

	// Implicit constructor: vector-like objects (std::list, std::vector,
	// std::set, etc)
	template<
	    class V,
	    std::enable_if_t<std::is_constructible_v<Json, typename V::value_type>,
	                     int> = 0>
	Json( const V &v ) : Json( array( v.begin(), v.end() ) )
	{
	}

	// This prevents Json(some_pointer) from accidentally producing a bool. Use
	// Json(bool(some_pointer)) if that behavior is desired.
	Json( void * ) = delete;

	// set to json nul
	void clear();

	// recursively remove all nul and empty strings, arrays and objects
	void clean();

	// Accessors
	Type type() const { return _type; }
	bool is_null() const { return type() == Type::NUL; }
	bool is_number() const { return type() == Type::NUMBER; }
	bool is_bool() const { return type() == Type::BOOL; }
	bool is_string() const { return type() == Type::STRING; }
	bool is_array() const { return type() == Type::ARRAY; }
	bool is_object() const { return type() == Type::OBJECT; }

	enum class NumberType
	{
		NOTANUMBER,
		INTEGER,
		INTEGER64,
		DOUBLE
	};
	NumberType number_type() const { return _numberType; }
	bool is_int() const { return number_type() == NumberType::INTEGER; }
	bool is_int64() const { return number_type() == NumberType::INTEGER64; }
	bool is_double() const { return number_type() == NumberType::DOUBLE; }

	// Return the enclosed value if this is a number, 0 otherwise. Note that
	// sjson does not distinguish between integer and non-integer numbers -
	// number_value() and int_value() can both be applied to a NUMBER-typed
	// object.
	double number_value() const;
	int32_t int_value() const;
	int64_t int64_value() const;

	// Return the enclosed value if this is a boolean, false otherwise.
	bool bool_value() const;
	// Return the enclosed string if this is a string, "" otherwise.
	const std::string &string_value() const;
	// Return the enclosed std::vector if this is an array, or an empty vector
	// otherwise.
	const array &array_items() const;
	// Return the enclosed std::map if this is an object, or an empty map
	// otherwise.
	const object &object_items() const;

	// Return the enclosed value as a double or 0
	double to_number_value() const;
	// Return the enclosed value as a int or 0
	int32_t to_int_value() const;
	int64_t to_int64_value() const;
	// Return the enclosed value as a bool or false
	bool to_bool_value() const;
	// Return the enclosed value as a string or empty string
	std::string to_string_value() const;

	// Return a reference to array[i] if this is an array, Json() otherwise.
	const Json &operator[]( size_t i ) const;
	// Return a reference to obj[key] if this is an object, Json() otherwise.
	const Json &operator[]( const std::string_view &key ) const;

	// Serialize.
	void dump( std::string &output ) const;
	std::string dump() const
	{
		std::string output;
		dump( output );
		return output;
	}

	// Parse. If parse fails, return Json() and assign an error message to err.
	static Json parse( const std::string_view &input,
	                   std::string &err,
	                   JsonParse strategy = JsonParse::STANDARD );
	static Json parse( const char *input,
	                   std::string &err,
	                   JsonParse strategy = JsonParse::STANDARD )
	{
		if ( input == nullptr )
		{
			err = "null input";
			return nullptr;
		}
		return parse( std::string_view( input ), err, strategy );
	}
	// Parse multiple objects, concatenated or separated by whitespace
	static std::vector<Json> parse_multi(
	    const std::string_view &input,
	    std::string_view::size_type &parser_stop_pos,
	    std::string &err,
	    JsonParse strategy = JsonParse::STANDARD );

	static inline std::vector<Json> parse_multi(
	    const std::string_view &input,
	    std::string &err,
	    JsonParse strategy = JsonParse::STANDARD )
	{
		std::string::size_type parser_stop_pos;
		return parse_multi( input, parser_stop_pos, err, strategy );
	}

	bool operator==( const Json &rhs ) const;
	bool operator<( const Json &rhs ) const;
	bool operator!=( const Json &rhs ) const { return !( *this == rhs ); }
	bool operator<=( const Json &rhs ) const { return !( rhs < *this ); }
	bool operator>( const Json &rhs ) const { return ( rhs < *this ); }
	bool operator>=( const Json &rhs ) const { return !( *this < rhs ); }
	/* has_shape(types, err)
	 *
	 * Return true if this is a JSON object and, for each item in types, has a
	 * field of the given type. If not, return false and set err to a
	 * descriptive message.
	 */
	typedef std::initializer_list<std::pair<std::string, Type>> shape;
	bool has_shape( const shape &types, std::string &err ) const;

private:
	union Storage
	{
		Storage() : all( 0 ) {}
		Storage( double v ) : d( v ) {}
		Storage( int32_t v ) : i32( v ) {}
		Storage( int64_t v ) : i64( v ) {}
		Storage( bool v ) : b( v ) {}
		Storage( const details::JsonValue *const v ) : p( v ) {}
		uint64_t all;
		const int32_t i32;
		const int64_t i64;
		const double d;
		const bool b;
		const details::JsonValue *const p; // ref counted
	} _data;
	static_assert( sizeof( Storage ) == sizeof( uint64_t ), "" );
	Type _type{Type::NUL};
	NumberType _numberType{NumberType::NOTANUMBER};
};

}

inline std::string to_string( const su::Json &v )
{
	return v.dump();
}

#endif
