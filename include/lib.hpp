#pragma once

#include <variant>
#include <unordered_map>
#include <vector>
#include <string>
#include <optional>

class JsonValue;

/**
 * Represents all possible valid JSON objects.
 */
using JsonObject = std::unordered_map<std::string, JsonValue>;

/**
 * Represents all possible valid JSON arrays.
 */
using JsonArray = std::vector<JsonValue>;

/**
 * Represents all possible valid non-null JSON values.
 */
using JsonValueVariant = std::variant<
    JsonObject,
    JsonArray,
    std::string,
    double,
    bool>;

/**
 * Represents all possible valid JSON values.
 */
class JsonValue
{
public:
    JsonValue(const JsonObject &v) : m_value(v) {}
    JsonValue(JsonObject &&v) : m_value(std::move(v)) {}

    JsonValue(const JsonArray &v) : m_value(v) {}
    JsonValue(JsonArray &&v) : m_value(std::move(v)) {}

    JsonValue(const std::string &v) : m_value(v) {}
    JsonValue(std::string &&v) : m_value(std::move(v)) {}

    JsonValue(double v) : m_value(v) {}
    explicit JsonValue(bool v) : m_value(v) {}
    JsonValue(std::nullptr_t) {}

    template <typename T>
    JsonValue(T *) = delete;

    JsonValue() = delete;

    /**
     * Retrieve a reference to the inner value if it exists.
     *
     * WARNING: Do not use this reference beyond the lifetime of the JsonValue containing it.
     * If you want an owned version, copy the value.
     *
     * @return a reference to the inner value if it exists.
     */
    std::optional<std::reference_wrapper<const JsonValueVariant>> value() const
    {
        if (!this->m_value)
        {
            return std::nullopt;
        }

        return std::cref(*this->m_value);
    }

    /**
     * Creates a JsonValue from a string containing valid JSON.
     *
     * @param json_str std::string containing valid JSON.
     * @return JsonValue containing that parsed JSON from json_str.
     */
    static JsonValue parse(const std::string_view json_str);

private:
    std::optional<JsonValueVariant> m_value;
};
