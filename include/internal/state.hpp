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
    enum StateExactType
    {
        True,
        False,
        Null,
    };

    struct StateValue;
    struct StateNumber;
    template <StateExactType ExactType>
    struct StateExact;
    struct StateString;
    struct StateArray;
    struct StateObject;

    using State = std::variant<StateValue,
                               StateNumber,
                               StateExact<True>,
                               StateExact<False>,
                               StateExact<Null>,
                               StateString,
                               StateArray,
                               StateObject>;

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

    template <StateExactType ExactType>
    struct StateExact
    {
        static constexpr const char *match()
        {
            switch (ExactType)
            {
            case True:
                return "true";
            case False:
                return "false";
            case Null:
                return "null";
            }

            return "";
        }

        static constexpr std::optional<StateExact<ExactType>> create_if_valid_start(char c);
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

}