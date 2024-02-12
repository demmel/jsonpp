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
#include "state.hpp"
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

    pda::StateOp<State> StateValue::transition(const char c)
    {
        if (std::isspace(c))
        {
            return pda::Noop{};
        }

        if (this->m_value.has_value())
        {
            return pda::Pop{};
        }

        if (auto new_state = tryCreateState<
                StateString, StateNumber, StateExact<True>, StateExact<False>, StateExact<Null>, StateObject, StateArray>(c))
        {
            return pda::Push<State>{
                std::visit([](auto &&s) -> State
                           { return State{s}; },
                           new_state.value()),
            };
        }

        throw std::runtime_error("Invalid JSON value");
    }

    StateFinalizationResult StateValue::finalize() const
    {
        if (!m_value.has_value())
        {
            return std::string("Unexpected end of input in JSON value");
        }
        return m_value.value();
    }

    std::optional<StateNumber> StateNumber::create_if_valid_start(char c)
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

    pda::StateOp<State> StateNumber::transition(const char c)
    {
        switch (this->state)
        {
        case NoDigits:
            if (c == '0')
            {
                this->state = Zero;
            }
            else if (std::isdigit(c))
            {
                this->state = SomeDigits;
            }
            else
            {
                throw std::runtime_error("Invalid character");
            }
            this->s.push_back(c);
            break;
        case SomeDigits:
            if (std::isdigit(c))
            {
            }
            else if (c == '.')
            {
                this->state = Dot;
            }
            else if (c == 'e' || c == 'E')
            {
                this->state = Exp;
            }
            else
            {
                return pda::Pop{};
            }
            this->s.push_back(c);
            break;
        case Zero:
            if (c == '.')
            {
                this->state = Dot;
            }
            else if (c == 'e' || c == 'E')
            {
                this->state = Exp;
            }
            else
            {
                return pda::Pop{};
            }
            this->s.push_back(c);
            break;
        case Dot:
            if (std::isdigit(c))
            {
                this->state = DotDigits;
            }
            else
            {
                throw std::runtime_error("Invalid character");
            }
            this->s.push_back(c);
            break;
        case DotDigits:
            if (std::isdigit(c))
            {
            }
            else if (c == 'e' || c == 'E')
            {
                this->state = Exp;
            }
            else
            {
                return pda::Pop{};
            }
            this->s.push_back(c);
            break;
        case Exp:
            if (std::isdigit(c))
            {
                this->state = ExpDigits;
            }
            else if (c == 'e' || c == 'E')
            {
                this->state = ExpSign;
            }
            else
            {
                throw std::runtime_error("Invalid character");
            }
            this->s.push_back(c);
            break;
        case ExpSign:
            if (std::isdigit(c))
            {
                this->state = ExpDigits;
            }
            else
            {
                throw std::runtime_error("Invalid character");
            }
            this->s.push_back(c);
            break;
        case ExpDigits:
            if (std::isdigit(c))
            {
                this->state = ExpDigits;
            }
            else
            {
                return pda::Pop{};
            }
            this->s.push_back(c);
            break;
        }

        return pda::Noop{};
    }

    StateFinalizationResult StateNumber::finalize() const
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

    template <StateExactType ExactType>
    constexpr std::optional<StateExact<ExactType>> StateExact<ExactType>::create_if_valid_start(char c)
    {
        if (c != match()[0])
        {
            return std::nullopt;
        }
        return StateExact<ExactType>{1};
    }

    template <StateExactType ExactType>
    pda::StateOp<State> StateExact<ExactType>::transition(const char c)
    {
        if (this->matched >= strlen(this->match()))
        {
            return pda::Pop{};
        }

        if (this->match()[this->matched] != c)
        {
            return pda::Reject{std::string("Expected '") + this->match()[this->matched] + std::string("' but got '") + c + std::string("'")};
        }

        ++this->matched;

        return pda::Noop{};
    }

    template <StateExactType ExactType>
    StateFinalizationResult StateExact<ExactType>::finalize() const
    {
        if (this->matched != strlen(this->match()))
        {
            return std::string("Unexpected end of input in JSON ") + this->match();
        }

        switch (ExactType)
        {
        case True:
            return JsonValue(true);
        case False:
            return JsonValue(false);
        case Null:
            return JsonValue(nullptr);
        }

        return std::string("Unhandled StateExact in finalize");
    }

    std::optional<StateString> StateString::create_if_valid_start(char c)
    {

        if (c != '"')
        {
            return std::nullopt;
        }
        return StateString{std::string()};
    }

    pda::StateOp<State> StateString::transition(const char c)
    {
        if (this->s.back() == '\\')
        {
            if (!(c == '"' || c == '\\' || c == '/' || c == 'b' || c == 'f' || c == 'n' || c == 'r' || c == 't' || c == 'u'))
            {
                throw std::runtime_error("Invalid escape sequence in JSON string");
            }
        }
        else
        {
            auto start_pos = this->s.size() - 6;
            if (start_pos < 0)
            {
                start_pos = 0;
            }
            auto unicode_escaoe = this->s.find("\\u", start_pos);
            if (unicode_escaoe != std::string::npos && unicode_escaoe >= this->s.size() - 6)
            {
                if (!std::isxdigit(c))
                {
                    throw std::runtime_error("Invalid hex digit in unicode escaped sequence in JSON string");
                }
            }
            else if (c == '"')
            {
                this->finished = true;
                return pda::Pop{false};
            }
        }

        this->s.push_back(c);

        return pda::Noop{};
    }

    StateFinalizationResult StateString::finalize() const
    {
        if (!this->finished)
        {
            return std::string("Missing closing \" on JSON string");
        }
        return JsonValue(this->s);
    }

    std::optional<StateArray> StateArray::create_if_valid_start(char c)
    {
        if (c != '[')
        {
            return std::nullopt;
        }
        return StateArray{};
    }

    pda::StateOp<State> StateArray::transition(const char c)
    {
        if (std::isspace(c))
        {
            return pda::Noop{};
        }
        else if (c == ']')
        {
            this->finished = true;
            return pda::Pop{false};
        }
        else if (this->need_comma)
        {
            if (c != ',')
            {
                throw std::runtime_error("Expected comma");
            }
            this->need_comma = false;
            return pda::Noop{};
        }
        else
        {
            return pda::Push<State>{StateValue{}, true};
        }
    }

    StateFinalizationResult StateArray::finalize() const
    {
        if (!this->finished)
        {
            return std::string("Missing closing ] on JSON array");
        }
        return JsonValue(this->values);
    }

    std::optional<StateObject> StateObject::create_if_valid_start(char c)
    {
        if (c != '{')
        {
            return std::nullopt;
        }
        return StateObject{};
    }

    pda::StateOp<State> StateObject::transition(const char c)
    {
        if (std::isspace(c))
        {
            return pda::Noop{};
        }
        else if (c == '}')
        {
            if (this->current_key)
            {
                throw std::runtime_error("JSON object missing value after key");
            }
            this->finished = true;
            return pda::Pop{false};
        }
        else if (this->need_comma)
        {
            if (c != ',')
            {
                throw std::runtime_error("Expected comma");
            }
            this->need_comma = false;
            return pda::Noop{};
        }
        else if (this->current_key)
        {
            if (c != ':')
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

    StateFinalizationResult StateObject::finalize() const
    {
        if (!this->finished)
        {
            return std::string("Missing closing } on JSON object");
        }
        return JsonValue(this->values);
    }

    struct StatePopOpVisitor
    {
        template <typename TCallback>
        std::optional<pda::Reject> tryFinalizePopped(TCallback callback)
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
            return callback(value);
        }

        std::optional<pda::Reject> operator()(StateValue &state)
        {
            return this->tryFinalizePopped(
                [&](auto value)
                {
                    state.m_value = value;
                    return std::nullopt;
                });
        }

        std::optional<pda::Reject> operator()(StateArray &state)
        {
            return this->tryFinalizePopped(
                [&](auto value)
                {
                    state.values.push_back(value);
                    state.need_comma = true;
                    return std::nullopt;
                });
        }

        std::optional<pda::Reject> operator()(StateObject &state)
        {
            return this->tryFinalizePopped(
                [&](auto value)
                {
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
                });
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
                    return std::visit(
                        [c](auto &state)
                        {
                            return state.transition(c);
                        },
                        state);
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