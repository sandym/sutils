/*
 *  attachable_tests.cpp
 *  sutils_tests
 *
 *  Created by Sandy Martel on 2008/05/30.
 *  Copyright 2015 Sandy Martel. All rights reserved.
 *
 *  quick reference:
 *
 *      TEST_ASSERT( condition [, message] )
 *          Assertions that a condition is true.
 *
 *      TEST_FAIL( message )
 *          Fails with the specified message.
 *
 *      TEST_ASSERT_EQUAL( expected, actual [, message] )
 *          Asserts that two values are equals.
 *
 *      TEST_ASSERT_NOT_EQUAL( not expected, actual [, message] )
 *          Asserts that two values are NOT equals.
 *
 *      TEST_ASSERT_EQUAL( expected, actual [, delta, message] )
 *          Asserts that two values are equals.
 *
 *      TEST_ASSERT_NOT_EQUAL( not expected, actual [, delta, message] )
 *          Asserts that two values are NOT equals.
 *
 *      TEST_ASSERT_THROW( expression, ExceptionType [, message] )
 *          Asserts that the given expression throws an exception of the specified type.
 *
 *      TEST_ASSERT_NO_THROW( expression [, message] )
 *          Asserts that the given expression does not throw any exceptions.
 */

#include "simple_tester.h"
#include "su_attachable.h"

struct attachable_tests
{
	attachable_tests();
	~attachable_tests();
	
	// declare all test cases here...
	void test_detach();
	void test_delete_attachable();
	void test_delete_attachment();

	private:
		// declare local members need for the test here
		su::attachable *_attachable;
};

REGISTER_TESTS( attachable_tests,
			   TEST_CASE(attachable_tests,test_detach),
			   TEST_CASE(attachable_tests,test_delete_attachable),
			   TEST_CASE(attachable_tests,test_delete_attachment) );

attachable_tests::attachable_tests()
{
	_attachable = new su::attachable;
}

attachable_tests::~attachable_tests()
{
	//	clean up local members
	delete _attachable;
	_attachable = nullptr;
}

// MARK: -
// MARK:  === test cases ===

class CheckDelete : public su::attachment
{
	public:
		CheckDelete( bool &o_wasDeleted )
			: _wasDeleted( o_wasDeleted )
		{
			_wasDeleted = false;
		}
		~CheckDelete()
		{
			_wasDeleted = true;
		}
	private:
		bool &_wasDeleted;
};

void attachable_tests::test_detach()
{
	bool wasDeleted = false;
	auto att = new CheckDelete( wasDeleted );
	_attachable->attach( "test", att );
	TEST_ASSERT( _attachable->get<CheckDelete>( "test" ) != nullptr );
	_attachable->detach( "test" );
	TEST_ASSERT( _attachable->get<CheckDelete>( "test" ) == nullptr );
	TEST_ASSERT( wasDeleted );
}

void attachable_tests::test_delete_attachable()
{
	bool wasDeleted = false;
	auto att = new CheckDelete( wasDeleted );
	_attachable->attach( "test", att );
	TEST_ASSERT( _attachable->get<CheckDelete>( "test" ) != nullptr );
	delete _attachable;
	_attachable = nullptr;
	TEST_ASSERT( wasDeleted );
}

void attachable_tests::test_delete_attachment()
{
	bool wasDeleted = false;
	auto att = new CheckDelete( wasDeleted );
	_attachable->attach( "test", att );
	TEST_ASSERT( _attachable->get<CheckDelete>( "test" ) != nullptr );
	delete att;
	TEST_ASSERT( _attachable->get<CheckDelete>( "test" ) == nullptr );
	TEST_ASSERT( wasDeleted );
}
