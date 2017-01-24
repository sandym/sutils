#ifndef H_SU_JSON
#define H_SU_JSON

#include "su_shim/string_view.h"
#include <string>
#include "su_flat_map.h"

namespace su
{
enum JsonParse
{
	STANDARD,
	COMMENTS
};

namespace details
{
struct JsonValue;
}

class Json final
{
  public:
	// Types
	enum Type
	{
		NUL, BOOL, NUMBER, STRING, ARRAY, OBJECT
	};

	// Array and object typedefs
	typedef std::vector<Json> array;
	typedef flat_map<std::string, Json> object;

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
	Json( int64_t value ); // NUMBER
	Json( bool value ); // BOOL
	Json( const std::string &value ); // STRING
	Json( std::string &&value ); // STRING
	Json( const char *value ); // STRING
	Json( const array &values ); // ARRAY
	Json( array &&values ); // ARRAY
	Json( const object &values ); // OBJECT
	Json( object &&values ); // OBJECT

	// Implicit constructor: anything with a to_json() function.
	template <class T, class = decltype( &T::to_json )>
	Json( const T &t ) : Json( t.to_json() ) {}

	// Implicit constructor: map-like objects (std::map, std::unordered_map, etc)
	template <class M, typename std::enable_if<std::is_constructible<std::string, typename M::key_type>::value &&
												   std::is_constructible<Json, typename M::mapped_type>::value,
											   int>::type = 0>
	Json( const M &m ) : Json( object( m.begin(), m.end() ) ){}

	// Implicit constructor: vector-like objects (std::list, std::vector, std::set, etc)
	template <class V, typename std::enable_if<std::is_constructible<Json, typename V::value_type>::value, int>::type = 0>
	Json( const V &v ) : Json( array( v.begin(), v.end() ) ){}

	// This prevents Json(some_pointer) from accidentally producing a bool. Use
	// Json(bool(some_pointer)) if that behavior is desired.
	Json( void * ) = delete;

	void clear();

	// Accessors
	inline Type type() const { return (Type)_type; }
	bool is_null() const { return type() == NUL; }
	bool is_number() const { return type() == NUMBER; }
	bool is_bool() const { return type() == BOOL; }
	bool is_string() const { return type() == STRING; }
	bool is_array() const { return type() == ARRAY; }
	bool is_object() const { return type() == OBJECT; }
	// Return the enclosed value if this is a number, 0 otherwise. Note that sjson does not
	// distinguish between integer and non-integer numbers - number_value() and int_value()
	// can both be applied to a NUMBER-typed object.
	double number_value() const;
	int int_value() const;
	int64_t int64_value() const;

	// Return the enclosed value if this is a boolean, false otherwise.
	bool bool_value() const;
	// Return the enclosed string if this is a string, "" otherwise.
	const std::string &string_value() const;
	// Return the enclosed std::vector if this is an array, or an empty vector otherwise.
	const array &array_items() const;
	// Return the enclosed std::map if this is an object, or an empty map otherwise.
	const object &object_items() const;

	// Return the enclosed value as a double or 0
	double to_number_value() const;
	// Return the enclosed value as a int or 0
	int to_int_value() const;
	// Return the enclosed value as a int64_t or 0
	int64_t to_int64_value() const;
	// Return the enclosed value as a bool or false
	bool to_bool_value() const;
	// Return the enclosed value as a string or empty string
	std::string to_string_value() const;

	// Return a reference to arr[i] if this is an array, Json() otherwise.
	const Json &operator[]( size_t i ) const;
	// Return a reference to obj[key] if this is an object, Json() otherwise.
	const Json &operator[]( const std::string &key ) const;

	// Serialize.
	void dump( std::string &output ) const;
	std::string dump() const
	{
		std::string output;
		dump( output );
		return output;
	}

	// Parse. If parse fails, return Json() and assign an error message to err.
	static Json parse( const su::string_view &input, std::string &err, JsonParse strategy = JsonParse::STANDARD );
	static Json parse( const char *input, std::string &err, JsonParse strategy = JsonParse::STANDARD )
	{
		if ( input == nullptr )
		{
			err = "null input";
			return nullptr;
		}
		return parse( su::string_view( input ), err, strategy );
	}
	// Parse multiple objects, concatenated or separated by whitespace
	static std::vector<Json> parse_multi( const su::string_view &input,
										  su::string_view::size_type &parser_stop_pos, std::string &err,
										  JsonParse strategy = JsonParse::STANDARD );

	static inline std::vector<Json> parse_multi( const su::string_view &input, std::string &err,
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
	 * Return true if this is a JSON object and, for each item in types, has a field of
	 * the given type. If not, return false and set err to a descriptive message.
	 */
	typedef std::initializer_list<std::pair<std::string, Type>> shape;
	bool has_shape( const shape &types, std::string &err ) const;

  private:
	union Stotage
	{
		Stotage() : all( 0 ){};
		Stotage( double v ) : all( 0 ) { d = v; };
		Stotage( int v ) : all( 0 ) { i = v; };
		Stotage( int64_t v ) : all( 0 ) { i64 = v; };
		Stotage( bool v ) : all( 0 ) { b = v; };
		Stotage( details::JsonValue *v ) : all( 0 ) { p = v; };
		uint64_t all;
		int64_t i64;
		double d;
		int i;
		bool b;
		details::JsonValue *p;
	} _data;
	static_assert( sizeof( Stotage ) == 8, "" );
	uint8_t _type{0};
	uint8_t _tag{0};
	
	enum tag_t
	{
		kPtr = 0x1,
		kDouble = 0x2,
		kInt = 0x4,
		kInt64 = 0x6,
		kNumberTypeMask = 0x6,
	};
	inline bool isPtr() const { return _tag&kPtr; }
	inline int numberType() const { return _tag&kNumberTypeMask; }
};
}

#endif
