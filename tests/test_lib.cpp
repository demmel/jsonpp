#include "gtest/gtest.h"

#include "lib.hpp"

template <typename T>
constexpr auto type_name()
{
    std::string_view name, prefix, suffix;
#ifdef __clang__
    name = __PRETTY_FUNCTION__;
    prefix = "auto type_name() [T = ";
    suffix = "]";
#elif defined(__GNUC__)
    name = __PRETTY_FUNCTION__;
    prefix = "constexpr auto type_name() [with T = ";
    suffix = "]";
#elif defined(_MSC_VER)
    name = __FUNCSIG__;
    prefix = "auto __cdecl type_name<";
    suffix = ">(void)";
#endif
    name.remove_prefix(prefix.size());
    name.remove_suffix(suffix.size());
    return name;
}

void assert_value_eq(const JsonValue &actual, const JsonValue &expected);

struct Visitor
{
    void operator()(const bool &actual, const bool &expected) const
    {
        ASSERT_EQ(actual, expected);
    }

    void operator()(const double &actual, const double &expected) const
    {
        ASSERT_EQ(actual, expected);
    }

    void operator()(const std::string &actual, const std::string &expected) const
    {
        ASSERT_EQ(actual, expected);
    }

    void operator()(const JsonArray &actual, const JsonArray &expected) const
    {
        ASSERT_EQ(actual.size(), expected.size());
        for (int i = 0; i < actual.size(); ++i)
        {
            assert_value_eq(actual[i], expected[i]);
        }
    }

    void operator()(const JsonObject &actual, const JsonObject &expected) const
    {
        ASSERT_EQ(actual.size(), expected.size());
        for (const auto &[key, value] : actual)
        {
            auto it = expected.find(key);
            ASSERT_NE(it, expected.end()) << "Actual contains key not in expected";
            assert_value_eq(value, it->second);
        }
    }

    template <typename T, typename U>
    void operator()(const T &actual, const U &expected) const
    {
        FAIL() << "Actual (" << ToJsonVisitor{}(actual) << ") has different type than expected (" << ToJsonVisitor{}(expected) << ")";
    }
};

void assert_variant_eq(const JsonValueVariant &actual, const JsonValueVariant &expected)
{
    std::visit(Visitor{}, actual, expected);
}

void assert_value_eq(const JsonValue &actual, const JsonValue &expected)
{
    auto expected_value = expected.value();
    if (expected_value)
    {
        auto expected_variant = expected_value->get();
        auto actual_value = actual.value();
        if (actual_value)
        {
            auto actual_variant = actual_value->get();
            assert_variant_eq(actual_variant, expected_variant);
        }
        else
        {
            FAIL() << "Expected non-null";
        }
    }
    else
    {
        ASSERT_FALSE(actual.value().has_value()) << "Expected null";
    }
}

TEST(LibTest, ParseNull)
{
    assert_value_eq(JsonValue::parse(std::string("null")), JsonValue(nullptr));
};

TEST(LibTest, ParseTrue)
{
    assert_value_eq(JsonValue::parse(std::string("true")), JsonValue(true));
};

TEST(LibTest, ParseFalse)
{
    assert_value_eq(JsonValue::parse(std::string("false")), JsonValue(false));
};

TEST(LibTest, ParseNumber)
{
    assert_value_eq(JsonValue::parse(std::string("1234")), JsonValue(1234.));
};

TEST(LibTest, ParseString)
{
    assert_value_eq(JsonValue::parse(std::string("\"Hello, world!\"")), JsonValue(std::string("Hello, world!")));
};

TEST(LibTest, ParseEmptyArray)
{
    assert_value_eq(JsonValue::parse(std::string("[]")), JsonValue(std::vector<JsonValue>{}));
};

TEST(LibTest, ParseEmptyObject)
{
    assert_value_eq(JsonValue::parse(std::string("{}")), JsonValue(std::unordered_map<std::string, JsonValue>{}));
};

TEST(LibTest, ParseStressTest)
{
    assert_value_eq(JsonValue::parse(
                        std::string("  {\"a\": {\"b\" : 123   ,  \"c\": \"asd\"}, \
                                \
                            \"d\": [1,2,3]    \
                            }")),
                    JsonValue(
                        {{std::string("a"),
                          JsonValue(
                              {{std::string("b"),
                                JsonValue(123.)},
                               {std::string("c"),
                                JsonValue(std::string("asd"))}})},
                         {std::string("d"),
                          JsonValue(
                              {JsonValue(1.),
                               JsonValue(2.),
                               JsonValue(3.)})}}));
};
