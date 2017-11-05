/*
 *  abstractparser.h
 *  sutils
 *
 *  Created by Sandy Martel on 07-04-18.
 *  Copyright (c) 2015年 Sandy Martel. All rights reserved.
 *
 * Permission to use, copy, modify, distribute, and sell this software for any
 * purpose is hereby granted without fee. The sotware is provided "AS-IS" and
 * without warranty of any kind, express, implied or otherwise.
 */

#ifndef H_SU_ABSTRACTPARSER
#define H_SU_ABSTRACTPARSER

#include <stack>
#include <iostream>
#include <sstream>
#include <unordered_map>

namespace su {

/*!
 * Token or lexical element class to help parsing files
 */
template<typename TOKEN_TYPE>
class token
{
public:
	token( const token & ) = default;
	token &operator=( const token & ) = default;

	token() = default;
	token( TOKEN_TYPE i_tok, const std::string_view &i_val )
		: _isValid( true ),
			_tokenid( i_tok ),
			_value( i_val )
	{}
	token( TOKEN_TYPE i_tok, std::string::value_type i_val )
		: _isValid( true ),
			_tokenid( i_tok ),
			_value( 1, i_val )
	{}

	token( token && ) = default;
	token &operator=( token &&i_other ) = default;

	void clear()
	{
		_properties.clear();
		_isValid = false;
	}
	
	bool isValid() const { return _isValid; }
	TOKEN_TYPE type() const { return _tokenid; }
	const std::string &value() const { return _value; }
	
	void setProperty( const std::string_view &i_name, const std::string_view &i_value )
	{
		_properties[i_name] = i_value;
	}
	std::string getProperty( const std::string_view &i_name ) const
	{
		auto it = _properties.find( i_name );
		if ( it != _properties.end() )
			return it->second;
		return std::string();
	}
	
private:
	bool _isValid = false;
	TOKEN_TYPE _tokenid;
	std::string _value;
	std::unordered_map<std::string,std::string> _properties;
};

class abstractparserbase
{
public:
	abstractparserbase( std::istream &i_str, const std::string_view &i_name = {} );
	virtual ~abstractparserbase() = default;

	/*!
	   @brief read from the current position to the end of the line.

	   @return  the string read
	*/
	std::string readLine();
	
	void throwTokenizerException() const;

protected:
	char nextNonSpaceChar();
	char nextChar();
	void putBackChar( char val );
	void skipToNextLine();
	bool readDoubleQuotedString( std::string &o_s );
	bool readNumber( std::string &o_s, bool &o_isFloat );

private:
	std::istream &_stream;
	std::string _name;

	// text file format
	enum AbstractParserStreamFormat_t { kutf8, kutf16BE, kutf16LE };
	AbstractParserStreamFormat_t _format;
	
	// parsing utility
	std::stack<char> _putbackCharList;
};

template<typename TOKEN_TYPE>
class abstractparser : public abstractparserbase
{
public:
	using token_type = token<TOKEN_TYPE>;

	abstractparser( std::istream &i_str, const std::string_view &i_name = {} );
	virtual ~abstractparser() = default;
	
	/*!
	   @brief get a token.

		get the next token of the stream, return false if no token found.
	   @param[out] o_token the token found or an invalid token
	   @return  true if o_token is valid
	*/
	bool nextToken( token_type &o_token );

	/*!
	   @brief put back a token.

			Put back a token in the stream, it will be the next token to be
			returned by nextToken().
	   @param[in]  i_token the token to return to the stream
	*/
	void putBackToken( token_type &i_token );

	/*!
	   @brief get a token of a specified type.

		get the next token of the stream, throw a parse error if no token found
		or the token is not of the specified type.
	   @param[out]  o_token the token found or an invalid token
	   @param[in]   i_forcedToken the type wanted
	*/
	void nextTokenForced( token_type &o_token, TOKEN_TYPE i_forcedToken );
	
	/*!
	   @brief get a token of a specified type.

		get the next token of the stream, return false if no token found or
			the token is not of the specified type.
	   @param[out] o_token   the token found or an invalid token
	   @param[in]  i_optionalToken the type wanted
	   @return   true if the specified type is found
	*/
	bool nextTokenOptional( token_type &o_token, TOKEN_TYPE i_optionalToken );
	
protected:
	std::stack<token_type> _putbackTokenList;
	
	virtual bool parseAToken( token_type &o_token ) = 0;
};

template<typename TOKEN_TYPE>
abstractparser<TOKEN_TYPE>::abstractparser( std::istream &i_str,
											const std::string_view &i_name )
	: abstractparserbase( i_str, i_name )
{
}

template<typename TOKEN_TYPE>
bool abstractparser<TOKEN_TYPE>::nextToken( token<TOKEN_TYPE> &o_token )
{
	// first look in the "put back" list
	if ( not _putbackTokenList.empty() )
	{
		o_token = std::move(_putbackTokenList.top());
		_putbackTokenList.pop();
		return true;
	}
	
	o_token = token<TOKEN_TYPE>();
	
	return parseAToken( o_token );
}

template<typename TOKEN_TYPE>
void abstractparser<TOKEN_TYPE>::putBackToken( token<TOKEN_TYPE> &i_token )
{
	if ( i_token.isValid() )
		_putbackTokenList.push( std::move(i_token) );
}

template<typename TOKEN_TYPE>
void abstractparser<TOKEN_TYPE>::nextTokenForced( token<TOKEN_TYPE> &o_token,
													TOKEN_TYPE i_forcedToken )
{
	if ( not nextTokenOptional( o_token, i_forcedToken ) )
		throwTokenizerException();
}

template<typename TOKEN_TYPE>
bool abstractparser<TOKEN_TYPE>::nextTokenOptional( token<TOKEN_TYPE> &o_token,
													TOKEN_TYPE i_optionalToken )
{
	if ( nextToken( o_token ) and o_token.type() == i_optionalToken )
		return true;
	else
	{
		putBackToken( o_token );
		return false;
	}
}

#if 0
#pragma mark -
#endif

enum linetoken_t
{
	line_tok_invalid,
	line_tok_line // xxxx
};

class lineparser : public abstractparser<linetoken_t>
{
public:
	lineparser( std::istream &i_str, const std::string_view &i_name );

protected:
	virtual bool parseAToken( token_type &o_token );
};

}

#endif
