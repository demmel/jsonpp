#pragma once

#include <variant>
#include <unordered_map>
#include <vector>
#include <string>
#include <optional>

struct JsonValue;

using JsonObject = std::unordered_map<std::string, JsonValue>;
using JsonArray = std::vector<JsonValue>;
using JsonValueVariant = std::optional<std::variant<
    JsonObject,
    JsonArray,
    std::string,
    double,
    bool>>;

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
    JsonValue(bool v) : m_value(v) {}

    JsonValue() = default;

    JsonValueVariant value()
    {
        return this->m_value;
    }

    static JsonValue parse(std::string &json_str);

private:
    JsonValueVariant m_value;
};
