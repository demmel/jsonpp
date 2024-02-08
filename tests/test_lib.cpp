#include "gtest/gtest.h"
#include "lib.hpp"

TEST(LibTest, ParseNull)
{
    auto parsed = JsonValue::parse(std::string("null"));

    EXPECT_EQ(parsed.value().has_value(), false);
};

TEST(LibTest, ParseTrue)
{
    auto parsed = JsonValue::parse(std::string("true"));

    EXPECT_EQ(parsed.value().has_value(), true);

    auto parsed_value = parsed.value().value();

    EXPECT_EQ(std::holds_alternative<bool>(parsed_value), true);

    auto parsed_bool = std::get<bool>(parsed_value);

    EXPECT_EQ(parsed_bool, true);
};

TEST(LibTest, ParseFalse)
{
    auto parsed = JsonValue::parse(std::string("false"));

    EXPECT_EQ(parsed.value().has_value(), true);

    auto parsed_value = parsed.value().value();

    EXPECT_EQ(std::holds_alternative<bool>(parsed_value), true);

    auto parsed_bool = std::get<bool>(parsed_value);

    EXPECT_EQ(parsed_bool, false);
};

TEST(LibTest, ParseNumber)
{
    auto parsed = JsonValue::parse(std::string("1234"));

    EXPECT_EQ(parsed.value().has_value(), true);

    auto parsed_value = parsed.value().value();

    EXPECT_EQ(std::holds_alternative<double>(parsed_value), true);

    auto parsed_double = std::get<double>(parsed_value);

    EXPECT_EQ(parsed_double, 1234.);
};

TEST(LibTest, ParseString)
{
    auto parsed = JsonValue::parse(std::string("\"Hello, world!\""));

    EXPECT_EQ(parsed.value().has_value(), true);

    auto parsed_value = parsed.value().value();

    EXPECT_EQ(std::holds_alternative<std::string>(parsed_value), true);

    auto parsed_string = std::get<std::string>(parsed_value);

    EXPECT_EQ(parsed_string, "Hello, world!");
};

TEST(LibTest, ParseEmptyArray)
{
    auto parsed = JsonValue::parse(std::string("[]"));

    EXPECT_EQ(parsed.value().has_value(), true);

    auto parsed_value = parsed.value().value();

    EXPECT_EQ(std::holds_alternative<JsonArray>(parsed_value), true);

    auto parsed_array = std::get<JsonArray>(parsed_value);

    EXPECT_EQ(parsed_array.size(), 0);
};

TEST(LibTest, ParseEmptyObject)
{
    auto parsed = JsonValue::parse(std::string("{}"));

    EXPECT_EQ(parsed.value().has_value(), true);

    auto parsed_value = parsed.value().value();

    EXPECT_EQ(std::holds_alternative<JsonObject>(parsed_value), true);

    auto parsed_object = std::get<JsonObject>(parsed_value);

    EXPECT_EQ(parsed_object.size(), 0);
};