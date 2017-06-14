#ifndef H_SU_SHIM_OPTIONAL
#define H_SU_SHIM_OPTIONAL

#include <stdexcept>

namespace su
{

class bad_optional_access : std::exception
{
public:
	virtual const char *what() const noexcept { return "bad_optional_access"; }
};

template<typename T>
class optional
{
public:
	optional(){}
	~optional() { reset(); }

	optional( const optional &rhs )
	{
		if ( rhs )
		{
			new(&_value) T(rhs._value);
			_engaged = true;
		}
	}
	optional &operator=( const optional &rhs )
	{
		if ( this != &rhs )
		{
			if ( not rhs )
				reset();
			else if ( _engaged )
				_value = rhs._value;
			else
			{
				new(&_value ) T(rhs._value);
				_engaged = true;
			}
		}
		return *this;
		
	}
	optional( optional &&rhs )
	{
		if ( rhs )
		{
			new(&_value ) T(std::move(rhs._value));
			_engaged = true;
		}
	}
	optional &operator=( optional &&rhs )
	{
		if ( this != &rhs )
		{
			if ( not rhs )
				reset();
			else if ( _engaged )
				_value = std::move(rhs._value);
			else
			{
				new(&_value ) T(std::move(rhs._value));
				_engaged = true;
			}
		}
		return *this;
	}
	optional( const T &rhs ) : _value( rhs ), _engaged( true ){}
	optional &operator=( const T &rhs )
	{
		if ( this != &rhs )
		{
			if ( _engaged )
				_value = rhs;
			else
				new(&_value ) T( rhs );
			_engaged = true;
		}
		return *this;
	}
	optional( T &&rhs ) : _value( std::move(rhs) ),  _engaged( true ){}
	optional &operator=( T &&rhs )
	{
		if ( _engaged )
			_value = std::move(rhs);
		else
			new(&_value ) T( std::move(rhs) );
		_engaged = true;
		return *this;
	}

	operator bool() const { return _engaged; }
	bool has_value() const { return _engaged; }

	T *operator->() { if ( not _engaged ) throw bad_optional_access(); return &_value; }
	const T *operator->() const { if ( not _engaged ) throw bad_optional_access(); return &_value; }
	T &operator*() { if ( not _engaged ) throw bad_optional_access(); return _value; }
	const T &operator*() const { if ( not _engaged ) throw bad_optional_access(); return _value; }

	T &value() { if ( not _engaged ) throw bad_optional_access(); return _value; }
	const T &value() const { if ( not _engaged ) throw bad_optional_access(); return _value; }

	T value_or( const T& i_default ) { return _engaged ? _value : i_default; }

	void reset()
	{
		if ( _engaged )
		{
			_value.~T();
			_engaged = false;
		}
	}

private:
	union
	{
		char _null;
		T _value;
	};
	bool _engaged = false;
};

}

#endif
