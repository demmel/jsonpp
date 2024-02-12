#pragma once

#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include "../lib.hpp"
#include "pda.hpp"

namespace jsonpp
{
    struct StateValue;
    struct StateNumber;
    struct StateTrue;
    struct StateFalse;
    struct StateString;
    struct StateArray;
    struct StateObject;
    struct StateNull;

    using State = std::variant<StateValue,
                               StateNumber,
                               StateTrue,
                               StateFalse,
                               StateString,
                               StateArray,
                               StateObject,
                               StateNull>;

    using StateFinalizationResult = std::variant<JsonValue, std::string>;

    struct StateValue
    {
        pda::StateOp<State> transition(const char c);
        StateFinalizationResult finalize() const;

        std::optional<JsonValue> m_value;
    };

    enum StateNumberState
    {
        NoDigits,
        Zero,
        SomeDigits,
        Dot,
        DotDigits,
        Exp,
        ExpSign,
        ExpDigits,
    };

    struct StateNumber
    {
        static std::optional<StateNumber> create_if_valid_start(char c);
        pda::StateOp<State> transition(const char c);
        StateFinalizationResult finalize() const;

        StateNumberState state;
        std::string s;
    };

    struct StateTrue
    {
        static inline const char *MATCH = "true";

        static std::optional<StateTrue> create_if_valid_start(char c);
        pda::StateOp<State> transition(const char c);
        StateFinalizationResult finalize() const;

        int matched;
    };

    struct StateFalse
    {
        static inline const char *MATCH = "false";

        static std::optional<StateFalse> create_if_valid_start(char c);
        pda::StateOp<State> transition(const char c);
        StateFinalizationResult finalize() const;

        int matched;
    };

    struct StateNull
    {
        static inline const char *MATCH = "null";

        static std::optional<StateNull> create_if_valid_start(char c);
        pda::StateOp<State> transition(const char c);
        StateFinalizationResult finalize() const;

        int matched;
    };

    struct StateString
    {
        static std::optional<StateString> create_if_valid_start(char c);
        pda::StateOp<State> transition(const char c);
        StateFinalizationResult finalize() const;

        std::string s;
        bool finished;
    };

    struct StateArray
    {
        static std::optional<StateArray> create_if_valid_start(char c);
        pda::StateOp<State> transition(const char c);
        StateFinalizationResult finalize() const;

        bool need_comma;
        JsonArray values;
        bool finished;
    };

    struct StateObject
    {
        static std::optional<StateObject> create_if_valid_start(char c);
        pda::StateOp<State> transition(const char c);
        StateFinalizationResult finalize() const;

        std::optional<std::string> current_key;
        bool need_comma;
        JsonObject values;
        bool finished;
    };

    using State = std::variant<StateValue,
                               StateNumber,
                               StateTrue,
                               StateFalse,
                               StateString,
                               StateArray,
                               StateObject,
                               StateNull>;
}