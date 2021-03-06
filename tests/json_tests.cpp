/*
 *  json_tests.cpp
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
#include "su_json.h"
#include "su_platform.h"
#include "su_resource_access.h"
#include <list>
#include <set>
#include <map>
#include <fstream>
#include <unordered_map>
#include <cstring>
#include <iostream>

using namespace su;
using std::string;

// Check that Json has the properties we want.
#define CHECK_TRAIT(x) static_assert(std::x::value, #x)
CHECK_TRAIT(is_nothrow_constructible<Json>);
CHECK_TRAIT(is_nothrow_default_constructible<Json>);
CHECK_TRAIT(is_copy_constructible<Json>);
CHECK_TRAIT(is_nothrow_move_constructible<Json>);
CHECK_TRAIT(is_copy_assignable<Json>);
CHECK_TRAIT(is_nothrow_move_assignable<Json>);
CHECK_TRAIT(is_nothrow_destructible<Json>);

struct json_tests
{
	json_tests();
	
	std::string kTwitter;
	std::string kCITM;
	std::string kCanada;

	//	declare all test cases here...
	void test_case_1();
	void test_case_2();
	void test_case_3();
};

REGISTER_TEST_SUITE( json_tests,
			   su::timed_test(), &json_tests::test_case_1,
			   su::timed_test(), &json_tests::test_case_2,
			   su::timed_test(), &json_tests::test_case_3 );

namespace {
std::string loadFile( const std::string &i_name )
{
	auto fpath = su::resource_access::get( i_name );
	
	std::ifstream f;
	fpath.fsopen( f );
	std::string s;
	while ( f )
	{
		char buf[4096];
		f.read( buf, 4096 );
		s.append( buf, f.gcount() );
	}
	return s;
}
}

json_tests::json_tests()
{
	kCanada = loadFile( "canada.json" );
	kCITM = loadFile( "citm_catalog.json" );
	kTwitter = loadFile( "twitter.json" );
}

// MARK: -
// MARK:  === test cases ===

#define SJSON_ENABLE_DR1467_CANARY 0

void json_tests::test_case_1()
{
    const string simple_test =
        R"({"k1":"v1", "k2":42, "k3":["a",123,true,false,null]})";

    string err;
    auto json = Json::parse(simple_test, err);

	TEST_ASSERT_EQUAL( json["k1"].string_value(), "v1" );
	TEST_ASSERT_EQUAL( json["k3"].dump(), "[\"a\",123,true,false,null]" );

	TEST_ASSERT_EQUAL( json["k3"].array_items().size(), 5 );

    const string comment_test = R"({
      // comment /* with nested comment */
      "a": 1,
      // comment
      // continued
      "b": "text",
      /* multi
         line
         comment */
      // and single-line comment
      "c": [1, 2, 3]
    })";

    string err_comment;
    auto json_comment = Json::parse(
      comment_test, err_comment, JsonParse::COMMENTS);
    TEST_ASSERT(err_comment.empty());

    string failing_comment_test = R"({
      /* bad comment
      "a": 1,
      // another comment to make C parsers which don't understand raw strings happy */
    })";

    string err_failing_comment;
    auto json_failing_comment = Json::parse(
      failing_comment_test, err_failing_comment, JsonParse::COMMENTS);
    TEST_ASSERT(err_failing_comment.empty());

    failing_comment_test = R"({
      / / bad comment })";

    json_failing_comment = Json::parse(
      failing_comment_test, err_failing_comment, JsonParse::COMMENTS);
    TEST_ASSERT( not err_failing_comment.empty());

    failing_comment_test = R"({// bad comment })";

    json_failing_comment = Json::parse(
      failing_comment_test, err_failing_comment, JsonParse::COMMENTS);
    TEST_ASSERT( not err_failing_comment.empty());

    failing_comment_test = R"({
          "a": 1
        }/)";

    json_failing_comment = Json::parse(
      failing_comment_test, err_failing_comment, JsonParse::COMMENTS);
    TEST_ASSERT( not err_failing_comment.empty());

    failing_comment_test = R"({/* bad
                                  comment *})";

    json_failing_comment = Json::parse(
      failing_comment_test, err_failing_comment, JsonParse::COMMENTS);
    TEST_ASSERT( not err_failing_comment.empty());

    std::list<int> l1 { 1, 2, 3 };
    std::vector<int> l2 { 1, 2, 3 };
    std::set<int> l3 { 1, 2, 3 };
    TEST_ASSERT_EQUAL(Json(l1), Json(l2));
    TEST_ASSERT_EQUAL(Json(l2), Json(l3));

    std::map<string, string> m1 { { "k1", "v1" }, { "k2", "v2" } };
    std::unordered_map<string, string> m2 { { "k1", "v1" }, { "k2", "v2" } };
    TEST_ASSERT_EQUAL(Json(m1), Json(m2));

    // Json literals
    Json obj = Json::object({
        { "k1", "v1" },
        { "k2", 42.0 },
        { "k3", Json::array({ "a", 123.0, true, false, nullptr }) },
    });

    //std::cout << "obj: " << obj.dump() << "\n";

    TEST_ASSERT_EQUAL(Json("a").number_value(), 0);
    TEST_ASSERT_EQUAL(Json("a").string_value(), "a");
    TEST_ASSERT_EQUAL(Json().number_value(), 0);

    TEST_ASSERT_EQUAL(obj, json);
    TEST_ASSERT_EQUAL(Json(42), Json(42.0));
    TEST_ASSERT_NOT_EQUAL(Json(42), Json(42.1));

    const string unicode_escape_test =
        R"([ "blah\ud83d\udca9blah\ud83dblah\udca9blah\u0000blah\u1234" ])";

    const char utf8[] = "blah" "\xf0\x9f\x92\xa9" "blah" "\xed\xa0\xbd" "blah"
                        "\xed\xb2\xa9" "blah" "\0" "blah" "\xe1\x88\xb4";

    Json uni = Json::parse(unicode_escape_test, err);
    TEST_ASSERT_EQUAL(uni[0].string_value().size(), (sizeof utf8) - 1);
    TEST_ASSERT_EQUAL(std::memcmp(uni[0].string_value().data(), utf8, sizeof utf8), 0);

    // Demonstrates the behavior change in Xcode 7 / Clang 3.7, introduced by DR1467
    // and described here: https://llvm.org/bugs/show_bug.cgi?id=23812
    if (SJSON_ENABLE_DR1467_CANARY) {
        Json nested_array = Json::array { Json::array { 1, 2, 3 } };
        TEST_ASSERT(nested_array.is_array());
        TEST_ASSERT_EQUAL(nested_array.array_items().size(), 1);
        TEST_ASSERT(nested_array.array_items()[0].is_array());
        TEST_ASSERT_EQUAL(nested_array.array_items()[0].array_items().size(), 3);
    }

    {
        const std::string good_json = R"( {"k1" : "v1"})";
        const std::string bad_json1 = good_json + " {";
        const std::string bad_json2 = good_json + R"({"k2":"v2", "k3":[)";
        struct TestMultiParse {
            std::string input;
            std::string::size_type expect_parser_stop_pos;
            size_t expect_not_empty_elms_count;
            Json expect_parse_res;
        } tests[] = {
            {" {", 0, 0, {}},
            {good_json, good_json.size(), 1, Json(std::map<string, string>{ { "k1", "v1" } })},
            {bad_json1, good_json.size() + 1, 1, Json(std::map<string, string>{ { "k1", "v1" } })},
            {bad_json2, good_json.size(), 1, Json(std::map<string, string>{ { "k1", "v1" } })},
            {"{}", 2, 1, Json::object{}},
        };
        for (const auto &tst : tests) {
            std::string::size_type parser_stop_pos;
            std::string err;
            auto res = Json::parse_multi(tst.input, parser_stop_pos, err);
            TEST_ASSERT_EQUAL(parser_stop_pos, tst.expect_parser_stop_pos);
            TEST_ASSERT_EQUAL(
                (size_t)std::count_if(res.begin(), res.end(),
                                      [](const Json& j) { return !j.is_null(); })
				, tst.expect_not_empty_elms_count);
            if (!res.empty()) {
                TEST_ASSERT_EQUAL(tst.expect_parse_res, res[0]);
            }
        }
    }
    Json my_json = Json::object {
        { "key1", "value1" },
        { "key2", false },
        { "key3", Json::array { 1, 2, 3 } },
    };
    std::string json_str = my_json.dump();
    //printf("%s\n", json_str.c_str());

    class Point {
    public:
        int x;
        int y;
        Point (int x, int y) : x(x), y(y) {}
        Json to_json() const { return Json::array { x, y }; }
    };

    std::vector<Point> points = { { 1, 2 }, { 10, 20 }, { 100, 200 } };
    std::string points_json = Json(points).dump();
	TEST_ASSERT_EQUAL( points_json, "[[1,2],[10,20],[100,200]]" );
}

struct Stat
{
	size_t arrayCount = 0;
	size_t elementCount = 0;
	size_t stringLength = 0;
	size_t objectCount = 0;
	size_t memberCount = 0;
	size_t stringCount = 0;
	size_t numberCount = 0;
	size_t trueCount = 0;
	size_t falseCount = 0;
	size_t nullCount = 0;
};

namespace {
void getStat( const su::Json &i_json, Stat &io_stat )
{
	switch ( i_json.type() )
	{
		case Json::Type::ARRAY:
			for ( auto const& i : i_json.array_items() )
				getStat( i, io_stat );
			io_stat.arrayCount++;
			io_stat.elementCount += i_json.array_items().size();
			break;
		case Json::Type::OBJECT:
			for ( auto const& i : i_json.object_items() )
			{
				getStat( i.second, io_stat );
				io_stat.stringLength += i.first.size();
			}
			io_stat.objectCount++;
			io_stat.memberCount += i_json.object_items().size();
			io_stat.stringCount += i_json.object_items().size();
			break;
		case Json::Type::STRING:
			io_stat.stringCount++;
			io_stat.stringLength += i_json.string_value().size();
			break;
		case Json::Type::NUMBER:
			io_stat.numberCount++;
			break;
		case Json::Type::BOOL:
			if ( i_json.bool_value() )
				io_stat.trueCount++;
			else
				io_stat.falseCount++;
			break;
		case Json::Type::NUL:
			io_stat.nullCount++;
			break;
	}
}
}

void json_tests::test_case_2()
{
	std::string err;
	Stat stat;

	auto json = su::Json::parse( kTwitter, err );
	TEST_ASSERT( err.empty() );
	getStat( json, stat );
	TEST_ASSERT_EQUAL( stat.objectCount, 1264 );
	TEST_ASSERT_EQUAL( stat.arrayCount, 1050 );
	TEST_ASSERT_EQUAL( stat.numberCount, 2109 );
	TEST_ASSERT_EQUAL( stat.stringCount, 18099 );
	TEST_ASSERT_EQUAL( stat.trueCount, 345 );
	TEST_ASSERT_EQUAL( stat.falseCount, 2446 );
	TEST_ASSERT_EQUAL( stat.nullCount, 1946 );
	TEST_ASSERT_EQUAL( stat.memberCount, 13345 );
	TEST_ASSERT_EQUAL( stat.elementCount, 568 );
	TEST_ASSERT_EQUAL( stat.stringLength, 367917 );
	
	err.clear();
	stat = Stat();
	json = su::Json::parse( kCITM, err );
	TEST_ASSERT( err.empty() );
	getStat( json, stat );
	TEST_ASSERT_EQUAL( stat.objectCount, 10937 );
	TEST_ASSERT_EQUAL( stat.arrayCount, 10451 );
	TEST_ASSERT_EQUAL( stat.numberCount, 14392 );
	TEST_ASSERT_EQUAL( stat.stringCount, 26604 );
	TEST_ASSERT_EQUAL( stat.trueCount, 0 );
	TEST_ASSERT_EQUAL( stat.falseCount, 0 );
	TEST_ASSERT_EQUAL( stat.nullCount, 1263 );
	TEST_ASSERT_EQUAL( stat.memberCount, 25869 );
	TEST_ASSERT_EQUAL( stat.elementCount, 11908 );
	TEST_ASSERT_EQUAL( stat.stringLength, 221379 );
	
	err.clear();
	stat = Stat();
	json = su::Json::parse( kCanada, err );
	TEST_ASSERT( err.empty() );
	getStat( json, stat );
	TEST_ASSERT_EQUAL( stat.objectCount, 4 );
	TEST_ASSERT_EQUAL( stat.arrayCount, 56045 );
	TEST_ASSERT_EQUAL( stat.numberCount, 111126 );
	TEST_ASSERT_EQUAL( stat.stringCount, 12 );
	TEST_ASSERT_EQUAL( stat.trueCount, 0 );
	TEST_ASSERT_EQUAL( stat.falseCount, 0 );
	TEST_ASSERT_EQUAL( stat.nullCount, 0 );
	TEST_ASSERT_EQUAL( stat.memberCount, 8 );
	TEST_ASSERT_EQUAL( stat.elementCount, 167170 );
	TEST_ASSERT_EQUAL( stat.stringLength, 90 );
}

void json_tests::test_case_3()
{
	std::string err;
	Stat stat;
	
	const char *roundtrip10 = R"END({"a":null,"foo":"bar"})END";
	
	auto json = su::Json::parse( roundtrip10, err );
	TEST_ASSERT( err.empty(), err );
	stat = Stat();
	getStat( json, stat );
	TEST_ASSERT_EQUAL( stat.objectCount, 1 );
	TEST_ASSERT_EQUAL( stat.arrayCount, 0 );
	TEST_ASSERT_EQUAL( stat.numberCount, 0 );
	TEST_ASSERT_EQUAL( stat.stringCount, 3 );
	TEST_ASSERT_EQUAL( stat.trueCount, 0 );
	TEST_ASSERT_EQUAL( stat.falseCount, 0 );
	TEST_ASSERT_EQUAL( stat.nullCount, 1 );
	TEST_ASSERT_EQUAL( stat.memberCount, 2 );
	TEST_ASSERT_EQUAL( stat.elementCount, 0 );
	TEST_ASSERT_EQUAL( stat.stringLength, 7 );
	
	auto s = json.dump();
	TEST_ASSERT_EQUAL( s, roundtrip10 );
}

