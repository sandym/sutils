/*
 *  json_tests.cpp
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
#include "su_json.h"
#include <list>
#include <set>
#include <map>
#include <unordered_map>
#include <cstring>

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
	//	declare all test cases here...
	void test_case_1();
};

REGISTER_TESTS( json_tests,
			   TEST_CASE(json_tests,test_case_1) );

// MARK: -
// MARK:  === test cases ===

#define SJSON_ENABLE_DR1467_CANARY 0

void json_tests::test_case_1()
{
    const string simple_test =
        R"({"k1":"v1", "k2":42, "k3":["a",123,true,false,null]})";

    string err;
    auto json = Json::parse(simple_test, err);

	TEST_ASSERT( json["k1"].string_value() == "v1" );
	TEST_ASSERT( json["k3"].dump() == "[\"a\",123,true,false,null]" );

	TEST_ASSERT( json["k3"].array_items().size() == 5 );

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
    TEST_ASSERT(Json(l1) == Json(l2));
    TEST_ASSERT(Json(l2) == Json(l3));

    std::map<string, string> m1 { { "k1", "v1" }, { "k2", "v2" } };
    std::unordered_map<string, string> m2 { { "k1", "v1" }, { "k2", "v2" } };
    TEST_ASSERT(Json(m1) == Json(m2));

    // Json literals
    Json obj = Json::object({
        { "k1", "v1" },
        { "k2", 42.0 },
        { "k3", Json::array({ "a", 123.0, true, false, nullptr }) },
    });

    //std::cout << "obj: " << obj.dump() << "\n";

    TEST_ASSERT(Json("a").number_value() == 0);
    TEST_ASSERT(Json("a").string_value() == "a");
    TEST_ASSERT(Json().number_value() == 0);

    TEST_ASSERT(obj == json);
    TEST_ASSERT(Json(42) == Json(42.0));
    TEST_ASSERT(Json(42) != Json(42.1));

    const string unicode_escape_test =
        R"([ "blah\ud83d\udca9blah\ud83dblah\udca9blah\u0000blah\u1234" ])";

    const char utf8[] = "blah" "\xf0\x9f\x92\xa9" "blah" "\xed\xa0\xbd" "blah"
                        "\xed\xb2\xa9" "blah" "\0" "blah" "\xe1\x88\xb4";

    Json uni = Json::parse(unicode_escape_test, err);
    TEST_ASSERT(uni[0].string_value().size() == (sizeof utf8) - 1);
    TEST_ASSERT(std::memcmp(uni[0].string_value().data(), utf8, sizeof utf8) == 0);

    // Demonstrates the behavior change in Xcode 7 / Clang 3.7, introduced by DR1467
    // and described here: https://llvm.org/bugs/show_bug.cgi?id=23812
    if (SJSON_ENABLE_DR1467_CANARY) {
        Json nested_array = Json::array { Json::array { 1, 2, 3 } };
        TEST_ASSERT(nested_array.is_array());
        TEST_ASSERT(nested_array.array_items().size() == 1);
        TEST_ASSERT(nested_array.array_items()[0].is_array());
        TEST_ASSERT(nested_array.array_items()[0].array_items().size() == 3);
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
            TEST_ASSERT(parser_stop_pos == tst.expect_parser_stop_pos);
            TEST_ASSERT(
                (size_t)std::count_if(res.begin(), res.end(),
                                      [](const Json& j) { return !j.is_null(); })
                == tst.expect_not_empty_elms_count);
            if (!res.empty()) {
                TEST_ASSERT(tst.expect_parse_res == res[0]);
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
	TEST_ASSERT( points_json == "[[1,2],[10,20],[100,200]]" );
}