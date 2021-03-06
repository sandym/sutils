/*
 *  attachable_tests.cpp
 *  sutils_tests
 *
 *  Created by Sandy Martel on 2008/05/30.
 *  Copyright 2015 Sandy Martel. All rights reserved.
 *
 *  quick reference:
 *
 *      TEST_ASSERT( condition )
 *          Assertions that a condition is true.
 *
 *      TEST_ASSERT_EQUAL( expected, actual )
 *          Asserts that two values are equals.
 *
 *      TEST_ASSERT_NOT_EQUAL( not expected, actual )
 *          Asserts that two values are NOT equals.
 */

#include "su_tests/simple_tests.h"
#include "su_attachable.h"

struct attachable_tests
{
	attachable_tests()
	{
		_attachable = std::make_unique<su::attachable>();
	}
	~attachable_tests() = default;
	
	class CheckDelete : public su::attachment
	{
	public:
		CheckDelete( bool &o_wasDeleted )
			: _wasDeleted( o_wasDeleted )
		{
			_wasDeleted = false;
		}
		~CheckDelete() { _wasDeleted = true; }
	private:
		bool &_wasDeleted;
	};

	void test_detach()
	{
		bool wasDeleted = false;
		_attachable->attach( "test", std::make_unique<CheckDelete>( wasDeleted ) );
		TEST_ASSERT_NOT_EQUAL( _attachable->get<CheckDelete>( "test" ), nullptr );
		_attachable->detach( "test" );
		TEST_ASSERT_EQUAL( _attachable->get<CheckDelete>( "test" ), nullptr );
		TEST_ASSERT( wasDeleted );
	}
	void test_delete_attachment()
	{
		bool wasDeleted = false;
		_attachable->attach( "test", std::make_unique<CheckDelete>( wasDeleted ) );
		TEST_ASSERT_NOT_EQUAL( _attachable->get<CheckDelete>( "test" ), nullptr );
		_attachable->detach( "test" );
		TEST_ASSERT_EQUAL( _attachable->get<CheckDelete>( "test" ), nullptr );
		TEST_ASSERT( wasDeleted );
	}

	private:
		// declare local members need for the test here
		std::unique_ptr<su::attachable> _attachable;
};

REGISTER_TEST_SUITE( attachable_tests,
			   &attachable_tests::test_detach,
			   &attachable_tests::test_delete_attachment );


