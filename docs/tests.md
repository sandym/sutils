## Simple unit test framework

Mostly unfinished. But aim at being super simple to use
- minimal set of macros
- minimal setup
- only one header one source
- can record result in an sqlite db
- record timing and detect performance regression (or try to...)
- work with cmake tests

Example:
```C++
#include "su/tests/simple_tests.h"

struct my_tests
{
	my_tests() { /* setup */ }
	~my_tests() { /* teardown */ }
	
	//	declare all test cases here...
	void test_case_1()
	{
		TEST_ASSERT( some_condition );
	}
	void test_case_2()
	{
		TEST_ASSERT_EQUAL( a, b );
	}
	void test_case_3()
	{
		TEST_NOT_ASSERT_EQUAL( a, b );
	}
};

REGISTER_TEST_SUITE( my_tests,
	&my_tests::test_case_1,
	&my_tests::test_case_2,
	su::timed_test(), &my_tests::test_case_3 );
```

Result would be something like:

```
Running all tests
Tests database: /Users/xxx/.simple_tests.db
Test suite: my tests
  test case 1 : ✔
  test case 2 : ✔
  test case 3 : ✔ (17684ns regression: 8273ns slower)
Success: 3/3
All good!
```
