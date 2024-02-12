#include "lib.hpp"

#include <variant>
#include <unordered_map>
#include <vector>
#include <string>
#include <optional>
#include <stdexcept>
#include <algorithm>
#include <iterator>

#include "utils.hpp"
#include "pda.hpp"

namespace jsonpp
{

    std::string ToJsonVisitor::operator()(const JsonObject &o) const
    {
        auto strs = std::vector<std::string>();
        strs.reserve(o.size());

        std::transform(o.begin(), o.end(), std::back_inserter(strs), [](auto kv)
                       { return kv.first + ":" + kv.second.json(); });

        return "{" + utils::join(strs, ',') + "}";
    }

    std::string ToJsonVisitor::operator()(const JsonArray &a) const
    {
        auto strs = std::vector<std::string>();
        strs.reserve(a.size());

        std::transform(a.begin(), a.end(), std::back_inserter(strs), [](auto elem)
                       { return elem.json(); });

        return "[" + utils::join(strs, ',') + "]";
    }

    std::string ToJsonVisitor::operator()(const std::string &s) const
    {
        return "\"" + s + "\"";
    }

    std::string ToJsonVisitor::operator()(const double &d) const
    {
        return std::to_string(d);
    }

    std::string ToJsonVisitor::operator()(const bool &b) const
    {
        return std::to_string(b);
    }

    using StateFinalizationResult = std::variant<JsonValue, std::string>;

    struct StateValue
    {
        StateFinalizationResult finalize() const
        {
            if (!m_value.has_value())
            {
                return std::string("Unexpected end of input in JSON value");
            }
            return m_value.value();
        }

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
        static std::optional<StateNumber> create_if_valid_start(char c)
        {
            StateNumberState state;
            if (c == '.')
            {
                state = StateNumberState::Dot;
            }
            else if (c == 'e' || c == 'E')
            {
                state = StateNumberState::Exp;
            }
            else if (c == '0')
            {
                state = StateNumberState::Zero;
            }
            else if (std::isdigit(c))
            {
                state = StateNumberState::SomeDigits;
            }
            else if (c == '-')
            {
                state = StateNumberState::NoDigits;
            }
            else
            {
                return std::nullopt;
            }

            return StateNumber{state, std::string(1, c)};
        }

        StateFinalizationResult finalize() const
        {
            switch (this->state)
            {
            case Zero:
            case SomeDigits:
            case DotDigits:
            case ExpDigits:
                return std::strtod(this->s.c_str(), NULL);
            default:
                return std::string("Unexpected end of input in JSON number");
            }
        }

        StateNumberState state;
        std::string s;
    };

    struct StateTrue
    {
        static inline const char *MATCH = "true";

        static std::optional<StateTrue> create_if_valid_start(char c)
        {
            if (c != MATCH[0])
            {
                return std::nullopt;
            }
            return StateTrue{1};
        }

        StateFinalizationResult finalize() const
        {
            if (this->matched != strlen(MATCH))
            {
                return std::string("Unexpected end of input in JSON true");
            }

            return JsonValue(true);
        }

        int matched;
    };

    struct StateFalse
    {
        static inline const char *MATCH = "false";

        static std::optional<StateFalse> create_if_valid_start(char c)
        {
            if (c != MATCH[0])
            {
                return std::nullopt;
            }
            return StateFalse{1};
        }

        StateFinalizationResult finalize() const
        {
            if (this->matched != strlen(MATCH))
            {
                return std::string("Unexpected end of input in JSON false");
            }

            return JsonValue(false);
        }

        int matched;
    };

    struct StateNull
    {
        static inline const char *MATCH = "null";

        static std::optional<StateNull> create_if_valid_start(char c)
        {
            if (c != MATCH[0])
            {
                return std::nullopt;
            }
            return StateNull{1};
        }

        StateFinalizationResult finalize() const
        {
            if (this->matched != strlen(MATCH))
            {
                return std::string("Unexpected end of input in JSON null");
            }

            return JsonValue(nullptr);
        }

        int matched;
    };

    struct StateString
    {
        static std::optional<StateString> create_if_valid_start(char c)
        {
            if (c != '"')
            {
                return std::nullopt;
            }
            return StateString{std::string()};
        }

        StateFinalizationResult finalize() const
        {
            if (!this->finished)
            {
                return std::string("Missing closing \" on JSON string");
            }
            return JsonValue(this->s);
        }

        std::string s;
        bool finished;
    };

    struct StateArray
    {
        static std::optional<StateArray> create_if_valid_start(char c)
        {
            if (c != '[')
            {
                return std::nullopt;
            }
            return StateArray{};
        }

        StateFinalizationResult finalize() const
        {
            if (!this->finished)
            {
                return std::string("Missing closing ] on JSON array");
            }
            return JsonValue(this->values);
        }

        bool need_comma;
        JsonArray values;
        bool finished;
    };

    struct StateObject
    {
        static std::optional<StateObject> create_if_valid_start(char c)
        {
            if (c != '{')
            {
                return std::nullopt;
            }
            return StateObject{};
        }

        StateFinalizationResult finalize() const
        {
            if (!this->finished)
            {
                return std::string("Missing closing } on JSON object");
            }
            return JsonValue(this->values);
        }

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

    template <std::size_t I, typename... Ts>
    constexpr std::optional<std::variant<Ts...>> tryCreateStateHelper(char c);

    template <typename... Ts>
    constexpr std::optional<std::variant<Ts...>> tryCreateState(char c)
    {
        return tryCreateStateHelper<0, Ts...>(c);
    }

    template <std::size_t I, typename... Ts>
    constexpr std::optional<std::variant<Ts...>> tryCreateStateHelper(char c)
    {
        if constexpr (I < sizeof...(Ts))
        {
            using T = std::tuple_element_t<I, std::tuple<Ts...>>;
            if (auto state = T::create_if_valid_start(c))
            {
                return state;
            }
            else
            {
                return tryCreateStateHelper<I + 1, Ts...>(c);
            }
        }
        else
        {
            return std::nullopt;
        }
    }

    struct StateTransitionVisitor
    {
        pda::StateOp<State> operator()(const StateValue &state) const
        {
            if (std::isspace(this->c))
            {
                return pda::Noop{};
            }

            if (state.m_value.has_value())
            {
                return pda::Pop{};
            }

            if (auto new_state = tryCreateState<
                    StateString, StateNumber, StateTrue, StateFalse, StateNull, StateObject, StateArray>(c))
            {
                return pda::Push<State>{std::visit([](auto &&s) -> State
                                                   { return State{s}; },
                                                   new_state.value())};
            }

            throw std::runtime_error("Invalid JSON value");
        }

        pda::StateOp<State> operator()(StateNumber &state) const
        {
            switch (state.state)
            {
            case NoDigits:
                if (this->c == '0')
                {
                    state.state = Zero;
                }
                else if (std::isdigit(this->c))
                {
                    state.state = SomeDigits;
                }
                else
                {
                    throw std::runtime_error("Invalid character");
                }
                state.s.push_back(this->c);
                break;
            case SomeDigits:
                if (std::isdigit(this->c))
                {
                }
                else if (this->c == '.')
                {
                    state.state = Dot;
                }
                else if (this->c == 'e' || this->c == 'E')
                {
                    state.state = Exp;
                }
                else
                {
                    return pda::Pop{};
                }
                state.s.push_back(this->c);
                break;
            case Zero:
                if (this->c == '.')
                {
                    state.state = Dot;
                }
                else if (this->c == 'e' || this->c == 'E')
                {
                    state.state = Exp;
                }
                else
                {
                    return pda::Pop{};
                }
                state.s.push_back(this->c);
                break;
            case Dot:
                if (std::isdigit(this->c))
                {
                    state.state = DotDigits;
                }
                else
                {
                    throw std::runtime_error("Invalid character");
                }
                state.s.push_back(this->c);
                break;
            case DotDigits:
                if (std::isdigit(this->c))
                {
                }
                else if (this->c == 'e' || this->c == 'E')
                {
                    state.state = Exp;
                }
                else
                {
                    return pda::Pop{};
                }
                state.s.push_back(this->c);
                break;
            case Exp:
                if (std::isdigit(this->c))
                {
                    state.state = ExpDigits;
                }
                else if (this->c == 'e' || this->c == 'E')
                {
                    state.state = ExpSign;
                }
                else
                {
                    throw std::runtime_error("Invalid character");
                }
                state.s.push_back(this->c);
                break;
            case ExpSign:
                if (std::isdigit(this->c))
                {
                    state.state = ExpDigits;
                }
                else
                {
                    throw std::runtime_error("Invalid character");
                }
                state.s.push_back(this->c);
                break;
            case ExpDigits:
                if (std::isdigit(this->c))
                {
                    state.state = ExpDigits;
                }
                else
                {
                    return pda::Pop{};
                }
                state.s.push_back(this->c);
                break;
            }

            return pda::Noop{};
        }

        pda::StateOp<State> operator()(StateTrue &state) const
        {
            if (state.matched >= strlen(state.MATCH))
            {
                return pda::Pop{};
            }

            ++state.matched;

            return pda::Noop{};
        }

        pda::StateOp<State> operator()(StateFalse &state) const
        {
            if (state.matched >= strlen(state.MATCH))
            {
                return pda::Pop{};
            }

            ++state.matched;

            return pda::Noop{};
        }

        pda::StateOp<State> operator()(StateString &state) const
        {
            if (state.s.back() == '\\')
            {
                if (!(c == '"' || c == '\\' || c == '/' || c == 'b' || c == 'f' || c == 'n' || c == 'r' || c == 't' || c == 'u'))
                {
                    throw std::runtime_error("Invalid escape sequence in JSON string");
                }
            }
            else
            {
                auto start_pos = state.s.size() - 6;
                if (start_pos < 0)
                {
                    start_pos = 0;
                }
                auto unicode_escaoe = state.s.find("\\u", start_pos);
                if (unicode_escaoe != std::string::npos && unicode_escaoe >= state.s.size() - 6)
                {
                    if (!std::isxdigit(this->c))
                    {
                        throw std::runtime_error("Invalid hex digit in unicode escaped sequence in JSON string");
                    }
                }
                else if (this->c == '"')
                {
                    state.finished = true;
                    return pda::Pop{false};
                }
            }

            state.s.push_back(this->c);

            return pda::Noop{};
        }

        pda::StateOp<State> operator()(StateArray &state) const
        {
            if (std::isspace(this->c))
            {
                return pda::Noop{};
            }
            else if (this->c == ']')
            {
                state.finished = true;
                return pda::Pop{false};
            }
            else if (state.need_comma)
            {
                if (this->c != ',')
                {
                    throw std::runtime_error("Expected comma");
                }
                state.need_comma = false;
                return pda::Noop{};
            }
            else
            {
                return pda::Push<State>{StateValue{}, true};
            }
        }

        pda::StateOp<State> operator()(StateObject &state) const
        {
            if (std::isspace(this->c))
            {
                return pda::Noop{};
            }
            else if (this->c == '}')
            {
                if (state.current_key)
                {
                    throw std::runtime_error("JSON object missing value after key");
                }
                state.finished = true;
                return pda::Pop{false};
            }
            else if (state.need_comma)
            {
                if (this->c != ',')
                {
                    throw std::runtime_error("Expected comma");
                }
                state.need_comma = false;
                return pda::Noop{};
            }
            else if (state.current_key)
            {
                if (this->c != ':')
                {
                    throw std::runtime_error("Expected colon");
                }
                return pda::Push<State>{StateValue{}};
            }
            else
            {
                auto next = StateString::create_if_valid_start(c);
                if (!next)
                {
                    throw std::runtime_error("Expected start of key");
                }
                return pda::Push<State>{next.value()};
            }
        }

        pda::StateOp<State> operator()(StateNull &state) const
        {
            if (state.matched >= strlen(state.MATCH))
            {
                return pda::Pop{};
            }

            ++state.matched;

            return pda::Noop{};
        }

        const char c;
    };

    struct StatePopOpVisitor
    {
        std::optional<pda::Reject> operator()(StateValue &state)
        {
            auto res = std::visit(
                [](auto &state)
                {
                    return state.finalize();
                },
                this->popped);
            auto error = std::get_if<std::string>(&res);
            if (error)
            {
                return pda::Reject{*error};
            }
            state.m_value = std::get<JsonValue>(res);
            return std::nullopt;
        }

        std::optional<pda::Reject> operator()(StateArray &state)
        {
            auto res = std::visit(
                [](auto &state)
                {
                    return state.finalize();
                },
                this->popped);
            auto error = std::get_if<std::string>(&res);
            if (error)
            {
                return pda::Reject{*error};
            }
            auto value = std::get<JsonValue>(res);

            state.values.push_back(value);
            state.need_comma = true;

            return std::nullopt;
        }

        std::optional<pda::Reject> operator()(StateObject &state)
        {
            auto res = std::visit(
                [](auto &state)
                {
                    return state.finalize();
                },
                this->popped);
            auto error = std::get_if<std::string>(&res);
            if (error)
            {
                return pda::Reject{*error};
            }
            auto value = std::get<JsonValue>(res);

            if (state.current_key)
            {
                state.values.insert({state.current_key.value(), value});
                state.current_key = std::nullopt;
                state.need_comma = true;
            }
            else
            {
                state.current_key = std::get<std::string>(value.value().value().get());
            }

            return std::nullopt;
        }

        template <typename T>
        std::optional<pda::Reject> operator()(T &)
        {
            return pda::Reject{"Cannot handle popped state"};
        }

        State popped;
    };

    JsonValue JsonValue::parse(const std::string_view json_str)
    {
        auto it = json_str.begin();
        auto pda = pda::PushdownAutomata<State, char>(StateValue{});

        for (size_t i = 0; i < json_str.size(); ++i)
        {
            auto c = json_str[i];
            auto res = pda.transition(
                c,
                [](auto &state, auto c)
                {
                    return std::visit(StateTransitionVisitor{c}, state);
                },
                [](auto &state, auto &popped)
                {
                    return std::visit(StatePopOpVisitor{popped}, state);
                });

            auto error = std::get_if<pda::TransitionError>(&res);
            if (error)
            {
                std::visit(
                    utils::inline_visitor{
                        [](pda::PoppedEmptyError)
                        {
                            throw std::runtime_error("Extraneous input after JSON");
                        },
                        [](pda::RejectedError error)
                        {
                            throw std::runtime_error(error.reason);
                        }},
                    *error);
            }
        }

        auto res =
            pda.finalize(
                [](auto &state)
                {
                    return std::visit(
                        [](auto &state) -> pda::FinalizeOp
                        {
                            auto res = state.finalize();
                            auto error = std::get_if<std::string>(&res);
                            if (error)
                            {
                                return pda::Reject{*error};
                            }
                            return pda::PopOrAccept{};
                        },
                        state);
                },
                [](auto &state, auto popped)
                {
                    return std::visit(StatePopOpVisitor{popped}, state);
                });

        if (auto error = std::get_if<pda::FinalizeError>(&res))
        {
            std::visit(
                utils::inline_visitor{
                    [](pda::RejectedError error)
                    {
                        throw std::runtime_error(error.reason);
                    }},
                *error);
        }

        auto final_state = std::get<State>(res);

        auto final_state_res = std::visit(
            [](auto &state)
            {
                return state.finalize();
            },
            final_state);
        auto error = std::get_if<std::string>(&final_state_res);
        if (error)
        {
            throw std::runtime_error(*error);
        }
        auto value = std::get<JsonValue>(final_state_res);

        return value;
    }
}