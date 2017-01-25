/*
 *  string_format_tests.cpp
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
#include "su_string_format.h"
#include "su_platform.h"
#include <cmath>
#include <cstdarg>

struct string_format_tests
{
	//	declare all test cases here...
	void test_case_format();
};

REGISTER_TESTS( string_format_tests,
				TEST_CASE(string_format_tests,test_case_format) );

// MARK: -
// MARK:  === test cases ===

std::string ustring_format_using_sprintf( const char *i_format, ... )
{
	char buf[4096];
	va_list ap;
	va_start( ap, i_format );
	vsprintf( buf, i_format, ap );
	va_end( ap );
	return std::string( buf );
}

#define PRINTF_TEST(...) TEST_ASSERT_EQUAL( su::format(__VA_ARGS__), ustring_format_using_sprintf(__VA_ARGS__) )

void string_format_tests::test_case_format()
{
	double pnumber = 789456123;
	
//	PRINTF_TEST( "%" );
	PRINTF_TEST( "%%" );
	PRINTF_TEST( "%%%" );
	PRINTF_TEST( "%%%%" );
	TEST_ASSERT_EQUAL( su::format( "%d%%" ), std::string("d%") ); // undefined behavior with printf

	/*
		%[index$][flags][width][.precision][length]specifier
	 
		[\d+$]			index for re-ordering
		[-]				left adjust
		[+]				+/- for signed value
		[space]			if no signed is to be printed, print a space
		[#]				float always with decimal point, octal always with 0 hex always with 0x
		[0]				pad numbers with 0
	 [\d+|*]			field width, * mean get it from the args
		[(.\d+)|(.*)]	precision, * mean get it from the args, default to 6
		[h]				next d, o, x or u is a short
		[l]				next d, o, x or u is a long
		[ll]			next d, o, x or u is a long long
		[z]				size_t
		[t]				ptrdiff_t
		type:
	 d		integer decimal
	 i		integer decimal
	 u		unsigned integer decimal
	 o		unsigned integer octal
	 x		unsigned integer hex 0x
	 X		unsigned integer hex 0X
	 f|F		float [-]ddd.ddd
	 e		float [-]d.ddde+dd or [-]d.ddde-dd
	 E		float [-]d.dddE+dd or [-]d.dddE-dd
	 g		d, f or e, whichever gives better greatest precision
	 G		d, f or E, whichever gives better greatest precision
	 c		character
	 s		string
	 p		pointer
	 @		any thing convertible to a string (std::to_string)
	 */

	PRINTF_TEST( "% *d\n", 16, -653 );
	PRINTF_TEST( "% *d\n", 16, 653 );
	PRINTF_TEST( "% 0*d\n", 16, -653 );
	PRINTF_TEST( "% 0*d\n", 16, 653 );
	PRINTF_TEST( "% 016d\n", -653 );
	PRINTF_TEST( "% 016d\n", 653 );
	PRINTF_TEST( "% 0d\n", -653 );
	PRINTF_TEST( "% 0d\n", 653 );
	PRINTF_TEST( "% 16d\n", -653 );
	PRINTF_TEST( "% 16d\n", 653 );
	PRINTF_TEST( "% d\n", -653 );
	PRINTF_TEST( "% d\n", 653 );
	PRINTF_TEST( "%*d\n", 16, -653 );
	PRINTF_TEST( "%*d\n", 16, 653 );
	PRINTF_TEST( "%+*d\n", 16, -653 );
	PRINTF_TEST( "%+*d\n", 16, 653 );
	PRINTF_TEST( "%+0*d\n", 16, -653 );
	PRINTF_TEST( "%+0*d\n", 16, 653 );
	PRINTF_TEST( "%+016d\n", -653 );
	PRINTF_TEST( "%+016d\n", 653 );
	PRINTF_TEST( "%+0d\n", -653 );
	PRINTF_TEST( "%+0d\n", 653 );
	PRINTF_TEST( "%+16d\n", -653 );
	PRINTF_TEST( "%+16d\n", 653 );
	PRINTF_TEST( "%+d\n", -653 );
	PRINTF_TEST( "%+d\n", 653 );
	PRINTF_TEST( "%- *d\n", 16, -653 );
	PRINTF_TEST( "%- *d\n", 16, 653 );
	PRINTF_TEST( "%- 16d\n", -653 );
	PRINTF_TEST( "%- 16d\n", 653 );
	PRINTF_TEST( "%- d\n", -653 );
	PRINTF_TEST( "%- d\n", 653 );
	PRINTF_TEST( "%-*d\n", 16, -653 );
	PRINTF_TEST( "%-*d\n", 16, 653 );
	PRINTF_TEST( "%-+*d\n", 16, -653 );
	PRINTF_TEST( "%-+*d\n", 16, 653 );
	PRINTF_TEST( "%-+16d\n", -653 );
	PRINTF_TEST( "%-+16d\n", 653 );
	PRINTF_TEST( "%-+d\n", -653 );
	PRINTF_TEST( "%-+d\n", 653 );
	PRINTF_TEST( "%-16d\n", -653 );
	PRINTF_TEST( "%-16d\n", 653 );
	PRINTF_TEST( "%-d\n", -653 );
	PRINTF_TEST( "%-d\n", 653 );
	PRINTF_TEST( "%0*d\n", 16, -653 );
	PRINTF_TEST( "%0*d\n", 16, 653 );
	PRINTF_TEST( "%016d\n", -653 );
	PRINTF_TEST( "%016d\n", 653 );
	PRINTF_TEST( "%0d\n", -653 );
	PRINTF_TEST( "%0d\n", 653 );
	PRINTF_TEST( "%16d\n", -653 );
	PRINTF_TEST( "%16d\n", 653 );
	PRINTF_TEST( "%d\n", -653 );
	PRINTF_TEST( "%d\n", 653 );

	PRINTF_TEST( "%*u\n", 16, 653 );
	PRINTF_TEST( "%*u\n", 16, 653 );
	PRINTF_TEST( "%-*u\n", 16, 653 );
	PRINTF_TEST( "%-*u\n", 16, 653 );
	PRINTF_TEST( "%-16u\n", 653 );
	PRINTF_TEST( "%-16u\n", 653 );
	PRINTF_TEST( "%-u\n", 653 );
	PRINTF_TEST( "%-u\n", 653 );
	PRINTF_TEST( "%0*u\n", 16, 653 );
	PRINTF_TEST( "%0*u\n", 16, 653 );
	PRINTF_TEST( "%016u\n", 653 );
	PRINTF_TEST( "%016u\n", 653 );
	PRINTF_TEST( "%0u\n", 653 );
	PRINTF_TEST( "%0u\n", 653 );
	PRINTF_TEST( "%16u\n", 653 );
	PRINTF_TEST( "%16u\n", 653 );
	PRINTF_TEST( "%u\n", 653 );
	PRINTF_TEST( "%u\n", 653 );

	PRINTF_TEST( "%s %d 123", "test", 3 );
#if !UPLATFORM_WIN

	PRINTF_TEST( "%2$s %1$d 123", 3, "test" );
	PRINTF_TEST( "%1$d %2$s 123", 3, "test" );
	PRINTF_TEST( "%2$s %1$d %2$s 123", 3, "test" );
#endif

	PRINTF_TEST( "%25s", "123" );
	PRINTF_TEST( "%-25s", "123" );
	PRINTF_TEST( "%d", 13 );
	PRINTF_TEST( "%+d", 13 );
	PRINTF_TEST( "% d", 13 );

	PRINTF_TEST( "%#f", 12.7 );
	PRINTF_TEST( "%#o", 12 );
	PRINTF_TEST( "%#x", 12 );
	PRINTF_TEST( "%#X", 12 );
	PRINTF_TEST( "%x", 12 );
	PRINTF_TEST( "%X", 12 );
	PRINTF_TEST( "%025i", 12 );
	PRINTF_TEST( "%0*i", 25, 12 );


	PRINTF_TEST( "%+#23.15e", pnumber );
	PRINTF_TEST( "%-#23.15e", pnumber );
	PRINTF_TEST( "%#23.15e", pnumber );
	PRINTF_TEST( "%#1.1g", pnumber );
	PRINTF_TEST( "% .80d", 1 );
	PRINTF_TEST( "%lld", ((uint64_t)0xffffffff)*0xffffffff );
//	PRINTF_TEST( "%I", 1 );
//	PRINTF_TEST( "%I0d", 1 );
	PRINTF_TEST( "% d", 1 );
	PRINTF_TEST( "%+ d", 1 );
	PRINTF_TEST( "%04c", '1' );
	PRINTF_TEST( "%-04c", '1' );
	PRINTF_TEST( "%#012x", 1 );
	PRINTF_TEST( "%#04.8x", 1 );
	PRINTF_TEST( "%#-08.2x", 1 );
	PRINTF_TEST( "%#08o", 1 );
	PRINTF_TEST( "%4s", "foo" );
	PRINTF_TEST( "%.1s", "foo" );
	PRINTF_TEST( "%.*s", 1, "foo" );
//	PRINTF_TEST( "'%*s'", -5, "foo" );
	PRINTF_TEST( "hello", 0 );
//	PRINTF_TEST( "%b", 1234 );
	PRINTF_TEST( "%3c",'a' );
	PRINTF_TEST( "%3d",1234 );
//	PRINTF_TEST( "%3h",1234 );
//	PRINTF_TEST( "%j%k%m%q%r%t%v%y%z", 0 );
	PRINTF_TEST( "%-1d",2 );
	PRINTF_TEST( "%2.4f",8.6 );
	PRINTF_TEST( "%0f",0.6 );
	PRINTF_TEST( "%.0f",0.6 );
	PRINTF_TEST( "%2.4e",8.6 );
	PRINTF_TEST( "% 2.4e",8.6 );
	PRINTF_TEST( "% 014.4e",8.6 );
	PRINTF_TEST( "% 2.4e",-8.6 );
	PRINTF_TEST( "%+2.4e",8.6 );
	PRINTF_TEST( "%2.4g",8.6 );
	PRINTF_TEST( "%-i",-1 );
	PRINTF_TEST( "%-i",1 );
	PRINTF_TEST( "%+i",1 );
	PRINTF_TEST( "%o",10 );
//	PRINTF_TEST( "%s",0 );
	PRINTF_TEST( "%s","%%%%" );
	PRINTF_TEST( "%u",-1 );
//	PRINTF_TEST( "%w",-1 );
//	PRINTF_TEST( "%h",-1 );
//	PRINTF_TEST( "%z",-1 );
//	PRINTF_TEST( "%j",-1 );
//	PRINTF_TEST( "%F",-1 );
//	PRINTF_TEST( "%H",-1 );
//	PRINTF_TEST( "x%cx", 0x100+'X' );
	PRINTF_TEST( "%%0", 0 );
	PRINTF_TEST( "%hx", 0x12345 );
//	PRINTF_TEST( "%hhx", 0x123 );
//	PRINTF_TEST( "%hhx", 0x12345 );

	double nan = 0.0;
	double inf = 1.0/nan;
	nan = std::sqrt(-1);
	PRINTF_TEST( "%lf", nan );
	PRINTF_TEST( "%lf", inf );
	PRINTF_TEST( "%le", nan );
	PRINTF_TEST( "%le", inf );
	PRINTF_TEST( "%lg", nan );
	PRINTF_TEST( "%lg", inf );
	PRINTF_TEST( "%010.2lf", nan );
	PRINTF_TEST( "%010.2lf", inf );
}
