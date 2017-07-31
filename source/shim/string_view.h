
#ifndef H_SHIM_STRING_VIEW
#define H_SHIM_STRING_VIEW

#include "su_platform.h"

#if UPLATFORM_WIN

#include <string>
#include <algorithm>

namespace su {

template<typename CHAR>
class basic_string_view
{
public:
	typedef CHAR value_type;
	typedef CHAR *pointer;
	typedef const CHAR *const_pointer;
	typedef CHAR &reference;
	typedef const CHAR &const_reference;
	typedef const CHAR *const_iterator;
	typedef const_iterator iterator;
//	typedef reverse_iterator<const_iterator> const_reverse_iterator;
//	typedef const_reverse_iterator reverse_iterator;
	typedef size_t size_type;
	typedef ptrdiff_t difference_type;
	static constexpr size_type npos = size_type(-1);

	basic_string_view() noexcept = default;
	basic_string_view( const basic_string_view & ) noexcept = default;
	basic_string_view &operator=( const basic_string_view & ) noexcept = default;
	basic_string_view( const std::basic_string<CHAR> &str ) noexcept : _data( str.data() ), _size( str.size() ){}
	basic_string_view( const CHAR *str ) : _data( str ), _size( ::strlen( str ) ){}
	basic_string_view( const CHAR *str, size_type len ) : _data( str ), _size( len ){}

	const_iterator begin() const noexcept { return _data; }
	const_iterator end() const noexcept { return _data + _size; }
	const_iterator cbegin() const noexcept { return _data; }
	const_iterator cend() const noexcept { return _data + _size; }
//	const_reverse_iterator rbegin() const noexcept;
//	const_reverse_iterator rend() const noexcept;
//	const_reverse_iterator crbegin() const noexcept;
//	const_reverse_iterator crend() const noexcept;

	size_type size() const noexcept { return _size; }
	size_type length() const noexcept { return _size; }
//	size_type max_size() const noexcept;
	bool empty() const noexcept { return _size == 0; }

	const_reference operator[]( size_type pos ) const { return _data[pos]; }
//	const_reference at( size_type pos ) const;
	const_reference front() const { return _data[0]; }
	const_reference back() const { return _data[_size-1]; }
	const_pointer data() const noexcept { return _data; }

	void clear() noexcept
	{
		_data = nullptr;
		_size = 0;
	}
	void remove_prefix( size_type n )
	{
		_data += n;
		_size -= n;
	}
	void remove_suffix( size_type n ) { _size -= n; }
//	void swap( basic_string_view &s ) noexcept;

//	explicit operator std::basic_string<CHAR>() const;

	operator std::basic_string<CHAR>() const { return std::basic_string<CHAR>( _data, _size ); }
//	std::basic_string<CHAR> to_string() const { return std::basic_string<CHAR>( _data, _size ); }

//	size_type copy( CHAR *s, size_type n, size_type pos = 0 ) const;

	basic_string_view substr( size_type pos = 0, size_type n = npos ) const
	{
		if ( pos > size() )
			throw std::out_of_range( "string_view::substr" );
		return basic_string_view( data() + pos, std::min( n, size() - pos ) );
	}
	int compare( basic_string_view s ) const noexcept
	{
		auto lhs_len = size();
		auto rhs_len = s.size();
		auto result = std::char_traits<CHAR>::compare( data(), s.data(), std::min( lhs_len, rhs_len ) );
		if ( result != 0 )
			return result;
		if ( lhs_len < rhs_len )
			return -1;
		if ( lhs_len > rhs_len )
			return 1;
		return 0;
	}
//	int compare( size_type pos1, size_type n1, basic_string_view s) const;
//	int compare( size_type pos1, size_type n1, basic_string_view s, size_type pos2, size_type n2 ) const;
//	int compare( const CHAR *s ) const;
//	int compare( size_type pos1, size_type n1, const CHAR *s ) const;
//	int compare( size_type pos1, size_type n1, const CHAR *s, size_type n2 ) const;
	size_type find( basic_string_view s, size_type pos = 0 ) const noexcept;
	size_type find( CHAR c, size_type pos = 0) const noexcept
	{
		for ( auto ptr = _data + pos; ptr < _data + _size; ++ptr )
		{
			if ( *ptr == c )
				return static_cast<size_type>( ptr - _data );
		}
		return npos;
	}
//	size_type find( const CHAR *s, size_type pos, size_type n) const;
//	size_type find( const CHAR *s, size_type pos = 0) const;
//	size_type rfind( basic_string_view s, size_type pos = npos) const noexcept;
	size_type rfind( CHAR c, size_type pos = npos) const noexcept
	{
		if ( empty() )
			return npos;
		if ( pos < size() )
			++pos;
		else
			pos = _size;
		for ( const CHAR *ptr = _data + pos; ptr != _data; )
		{
			if ( *--ptr == c )
				return static_cast<size_type>( ptr - _data );
		}
		return npos;
	}
//	size_type rfind( const CHAR *s, size_type pos, size_type n) const;
//	size_type rfind( const CHAR *s, size_type pos = npos) const;
	size_type find_first_of( basic_string_view s, size_type pos = 0) const noexcept
	{
		for ( auto ptr = _data + pos; ptr < _data + _size; ++ptr )
		{
			if ( s.find( *ptr ) != npos )
				return static_cast<size_type>( ptr - _data );
		}
		return npos;
	}
//	size_type find_first_of( CHAR c, size_type pos = 0) const noexcept;
//	size_type find_first_of( const CHAR *s, size_type pos, size_type n) const;
	size_type find_first_of( const CHAR *s, size_type pos = 0 ) const { return find_first_of( basic_string_view( s ), pos ); }
//	size_type find_last_of( basic_string_view s, size_type pos = npos) const noexcept;
	size_type find_last_of( CHAR c, size_type pos = npos ) const noexcept { return rfind( c, pos ); }
//	size_type find_last_of( const CHAR *s, size_type pos, size_type n) const;
//	size_type find_last_of( const CHAR *s, size_type pos = npos) const;
	size_type find_first_not_of( basic_string_view s, size_type pos = 0) const noexcept
	{
		for ( auto ptr = _data + pos; ptr < _data + _size; ++ptr )
		{
			if ( s.find( *ptr ) == npos )
				return static_cast<size_type>( ptr - _data );
		}
		return npos;
	}
//	size_type find_first_not_of( CHAR c, size_type pos = 0) const noexcept;
//	size_type find_first_not_of( const CHAR *s, size_type pos, size_type n) const;
	size_type find_first_not_of( const CHAR *s, size_type pos = 0 ) const { return find_first_not_of( basic_string_view( s ), pos ); }
	size_type find_last_not_of( basic_string_view s, size_type pos = npos) const noexcept
	{
		if ( empty() )
			return npos;
		if ( pos < size() )
			++pos;
		else
			pos = _size;
		for ( const CHAR *ptr = _data + pos; ptr != _data; )
		{
			if ( s.find( *--ptr ) == npos )
				return static_cast<size_type>( ptr - _data );
		}
		return npos;
	}
//	size_type find_last_not_of( CHAR c, size_type pos = npos) const noexcept;
//	size_type find_last_not_of( const CHAR *s, size_type pos, size_type n) const;
	size_type find_last_not_of( const CHAR *s, size_type pos = npos ) const { return find_last_not_of( basic_string_view( s ), pos ); }

private:
	const_pointer _data = nullptr;
	size_type _size = 0;
};

template<class CHAR>
bool operator==( basic_string_view<CHAR> lhs, basic_string_view<CHAR> rhs ) noexcept
{
	return lhs.compare( rhs ) == 0;
}
template<class CHAR>
bool operator==( const std::basic_string<CHAR> &lhs, basic_string_view<CHAR> rhs ) noexcept
{
	return lhs.compare( 0, lhs.size(), rhs.data(), rhs.size() ) == 0;
}
template<class CHAR>
bool operator==( basic_string_view<CHAR> lhs, const std::basic_string<CHAR> &rhs ) noexcept
{
	return lhs.compare( rhs ) == 0;
}
//template<class CHAR>
//bool operator==( const char *lhs, basic_string_view<CHAR> rhs ) noexcept;
template<class CHAR>
bool operator==( basic_string_view<CHAR> lhs, const char *rhs ) noexcept
{
	return lhs.compare( rhs ) == 0;
}

//template<class CHAR>
//bool operator!=( basic_string_view<CHAR> lhs, basic_string_view<CHAR> rhs ) noexcept;
//template<class CHAR>
//bool operator<( basic_string_view<CHAR> lhs, basic_string_view<CHAR> rhs ) noexcept;
//template<class CHAR>
//bool operator>( basic_string_view<CHAR> lhs, basic_string_view<CHAR> rhs ) noexcept;
//template<class CHAR>
//bool operator<=( basic_string_view<CHAR> lhs, basic_string_view<CHAR> rhs ) noexcept;
//template<class CHAR>
//bool operator>=( basic_string_view<CHAR> lhs, basic_string_view<CHAR> rhs ) noexcept;

using string_view = basic_string_view<char>;
using u16string_view = basic_string_view<char16_t>;

}

typedef intptr_t ssize_t;

#else

#include <string>
#include <algorithm>

namespace su {

template<typename CHAR>
class basic_string_view
{
public:
	typedef CHAR value_type;
	typedef CHAR *pointer;
	typedef const CHAR *const_pointer;
	typedef CHAR &reference;
	typedef const CHAR &const_reference;
	typedef const CHAR *const_iterator;
	typedef const_iterator iterator;
//	typedef reverse_iterator<const_iterator> const_reverse_iterator;
//	typedef const_reverse_iterator reverse_iterator;
	typedef size_t size_type;
	typedef ptrdiff_t difference_type;
	static constexpr size_type npos = size_type(-1);

	basic_string_view() noexcept = default;
	basic_string_view( const basic_string_view & ) noexcept = default;
	basic_string_view &operator=( const basic_string_view & ) noexcept = default;
	basic_string_view( const std::basic_string<CHAR> &str ) noexcept : _data( str.data() ), _size( str.size() ){}
	basic_string_view( const CHAR *str ) : _data( str ), _size( ::strlen( str ) ){}
	basic_string_view( const CHAR *str, size_type len ) : _data( str ), _size( len ){}

	const_iterator begin() const noexcept { return _data; }
	const_iterator end() const noexcept { return _data + _size; }
	const_iterator cbegin() const noexcept { return _data; }
	const_iterator cend() const noexcept { return _data + _size; }
//	const_reverse_iterator rbegin() const noexcept;
//	const_reverse_iterator rend() const noexcept;
//	const_reverse_iterator crbegin() const noexcept;
//	const_reverse_iterator crend() const noexcept;

	size_type size() const noexcept { return _size; }
	size_type length() const noexcept { return _size; }
//	size_type max_size() const noexcept;
	bool empty() const noexcept { return _size == 0; }

	const_reference operator[]( size_type pos ) const { return _data[pos]; }
//	const_reference at( size_type pos ) const;
	const_reference front() const { return _data[0]; }
	const_reference back() const { return _data[_size-1]; }
	const_pointer data() const noexcept { return _data; }

	void clear() noexcept
	{
		_data = nullptr;
		_size = 0;
	}
	void remove_prefix( size_type n )
	{
		_data += n;
		_size -= n;
	}
	void remove_suffix( size_type n ) { _size -= n; }
//	void swap( basic_string_view &s ) noexcept;

//	explicit operator std::basic_string<CHAR>() const;

	operator std::basic_string<CHAR>() const { return std::basic_string<CHAR>( _data, _size ); }
//	std::basic_string<CHAR> to_string() const { return std::basic_string<CHAR>( _data, _size ); }

//	size_type copy( CHAR *s, size_type n, size_type pos = 0 ) const;

	basic_string_view substr( size_type pos = 0, size_type n = npos ) const
	{
		if ( pos > size() )
			throw std::out_of_range( "string_view::substr" );
		return basic_string_view( data() + pos, std::min( n, size() - pos ) );
	}
	int compare( basic_string_view s ) const noexcept
	{
		auto lhs_len = size();
		auto rhs_len = s.size();
		auto result = std::char_traits<CHAR>::compare( data(), s.data(), std::min( lhs_len, rhs_len ) );
		if ( result != 0 )
			return result;
		if ( lhs_len < rhs_len )
			return -1;
		if ( lhs_len > rhs_len )
			return 1;
		return 0;
	}
//	int compare( size_type pos1, size_type n1, basic_string_view s) const;
//	int compare( size_type pos1, size_type n1, basic_string_view s, size_type pos2, size_type n2 ) const;
//	int compare( const CHAR *s ) const;
//	int compare( size_type pos1, size_type n1, const CHAR *s ) const;
//	int compare( size_type pos1, size_type n1, const CHAR *s, size_type n2 ) const;
	size_type find( basic_string_view s, size_type pos = 0 ) const noexcept;
	size_type find( CHAR c, size_type pos = 0) const noexcept
	{
		for ( auto ptr = _data + pos; ptr < _data + _size; ++ptr )
		{
			if ( *ptr == c )
				return static_cast<size_type>( ptr - _data );
		}
		return npos;
	}
//	size_type find( const CHAR *s, size_type pos, size_type n) const;
//	size_type find( const CHAR *s, size_type pos = 0) const;
//	size_type rfind( basic_string_view s, size_type pos = npos) const noexcept;
	size_type rfind( CHAR c, size_type pos = npos) const noexcept
	{
		if ( empty() )
			return npos;
		if ( pos < size() )
			++pos;
		else
			pos = _size;
		for ( const CHAR *ptr = _data + pos; ptr != _data; )
		{
			if ( *--ptr == c )
				return static_cast<size_type>( ptr - _data );
		}
		return npos;
	}
//	size_type rfind( const CHAR *s, size_type pos, size_type n) const;
//	size_type rfind( const CHAR *s, size_type pos = npos) const;
	size_type find_first_of( basic_string_view s, size_type pos = 0) const noexcept
	{
		for ( auto ptr = _data + pos; ptr < _data + _size; ++ptr )
		{
			if ( s.find( *ptr ) != npos )
				return static_cast<size_type>( ptr - _data );
		}
		return npos;
	}
//	size_type find_first_of( CHAR c, size_type pos = 0) const noexcept;
//	size_type find_first_of( const CHAR *s, size_type pos, size_type n) const;
	size_type find_first_of( const CHAR *s, size_type pos = 0 ) const { return find_first_of( basic_string_view( s ), pos ); }
//	size_type find_last_of( basic_string_view s, size_type pos = npos) const noexcept;
	size_type find_last_of( CHAR c, size_type pos = npos ) const noexcept { return rfind( c, pos ); }
//	size_type find_last_of( const CHAR *s, size_type pos, size_type n) const;
//	size_type find_last_of( const CHAR *s, size_type pos = npos) const;
	size_type find_first_not_of( basic_string_view s, size_type pos = 0) const noexcept
	{
		for ( auto ptr = _data + pos; ptr < _data + _size; ++ptr )
		{
			if ( s.find( *ptr ) == npos )
				return static_cast<size_type>( ptr - _data );
		}
		return npos;
	}
//	size_type find_first_not_of( CHAR c, size_type pos = 0) const noexcept;
//	size_type find_first_not_of( const CHAR *s, size_type pos, size_type n) const;
	size_type find_first_not_of( const CHAR *s, size_type pos = 0 ) const { return find_first_not_of( basic_string_view( s ), pos ); }
	size_type find_last_not_of( basic_string_view s, size_type pos = npos) const noexcept
	{
		if ( empty() )
			return npos;
		if ( pos < size() )
			++pos;
		else
			pos = _size;
		for ( const CHAR *ptr = _data + pos; ptr != _data; )
		{
			if ( s.find( *--ptr ) == npos )
				return static_cast<size_type>( ptr - _data );
		}
		return npos;
	}
//	size_type find_last_not_of( CHAR c, size_type pos = npos) const noexcept;
//	size_type find_last_not_of( const CHAR *s, size_type pos, size_type n) const;
	size_type find_last_not_of( const CHAR *s, size_type pos = npos ) const { return find_last_not_of( basic_string_view( s ), pos ); }

private:
	const_pointer _data = nullptr;
	size_type _size = 0;
};

template<class CHAR>
bool operator==( basic_string_view<CHAR> lhs, basic_string_view<CHAR> rhs ) noexcept
{
	return lhs.compare( rhs ) == 0;
}
template<class CHAR>
bool operator==( const std::basic_string<CHAR> &lhs, basic_string_view<CHAR> rhs ) noexcept
{
	return lhs.compare( 0, lhs.size(), rhs.data(), rhs.size() ) == 0;
}
template<class CHAR>
bool operator==( basic_string_view<CHAR> lhs, const std::basic_string<CHAR> &rhs ) noexcept
{
	return lhs.compare( rhs ) == 0;
}
//template<class CHAR>
//bool operator==( const char *lhs, basic_string_view<CHAR> rhs ) noexcept;
template<class CHAR>
bool operator==( basic_string_view<CHAR> lhs, const char *rhs ) noexcept
{
	return lhs.compare( rhs ) == 0;
}

//template<class CHAR>
//bool operator!=( basic_string_view<CHAR> lhs, basic_string_view<CHAR> rhs ) noexcept;
template<class CHAR>
bool operator<( basic_string_view<CHAR> lhs, basic_string_view<CHAR> rhs ) noexcept
{
	return lhs.compare( rhs ) < 0;
}
//template<class CHAR>
//bool operator>( basic_string_view<CHAR> lhs, basic_string_view<CHAR> rhs ) noexcept;
//template<class CHAR>
//bool operator<=( basic_string_view<CHAR> lhs, basic_string_view<CHAR> rhs ) noexcept;
//template<class CHAR>
//bool operator>=( basic_string_view<CHAR> lhs, basic_string_view<CHAR> rhs ) noexcept;

using string_view = basic_string_view<char>;
using u16string_view = basic_string_view<char16_t>;

}

#endif

#endif
