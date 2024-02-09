#include "lib.hpp"

#include <stdexcept>

// Define a helper type to combine all lambdas into a single visitable type
template <class... Ts>
struct inline_visit : Ts...
{
    using Ts::operator()...;
};
template <class... Ts>
inline_visit(Ts...) -> inline_visit<Ts...>;

struct StateValue
{
    std::optional<JsonValue> value;
};

enum StateNumberState
{
    NO_DIGITS,
    ZERO,
    SOME_DIGIT,
    DOT,
    DOT_DIGITS,
    EXP,
    EXP_SIGN,
    EXP_DIGITS,
};

struct StateNumber
{
    static std::optional<StateNumber> create_if_valid_start(char c)
    {
        StateNumberState state;
        if (c == '.')
        {
            state = StateNumberState::DOT;
        }
        else if (c == 'e' || c == 'E')
        {
            state = StateNumberState::EXP;
        }
        else if (c == '0')
        {
            state = StateNumberState::ZERO;
        }
        else if (std::isdigit(c))
        {
            state = StateNumberState::SOME_DIGIT;
        }
        else if (c == '-')
        {
            state = StateNumberState::NO_DIGITS;
        }
        else
        {
            return std::nullopt;
        }

        return StateNumber{state, std::string(1, c)};
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

    std::string s;
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

    JsonArray values;
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

    JsonObject values;
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

    int matched;
};

using State = std::variant<StateValue,
                           StateNumber,
                           StateTrue,
                           StateFalse,
                           StateString,
                           StateArray,
                           StateObject,
                           StateNull>;

struct Noop
{
};
struct Push
{
    State state;
};
struct Pop
{
    JsonValue value;
    bool redo = true;
};

using StateOp = std::variant<Noop, Push, Pop>;

struct StateCharVisitor
{
    StateOp operator()(const StateValue &state) const
    {
        if (std::isspace(this->c))
        {
            return Noop{};
        }

        if (state.value.has_value())
        {
            return Pop{state.value.value()};
        }

        if (auto new_state = StateNumber::create_if_valid_start(c))
        {
            return Push{State{new_state.value()}};
        }

        if (auto new_state = StateTrue::create_if_valid_start(c))
        {
            return Push{State{new_state.value()}};
        }

        if (auto new_state = StateFalse::create_if_valid_start(c))
        {
            return Push{State{new_state.value()}};
        }

        if (auto new_state = StateString::create_if_valid_start(c))
        {
            return Push{State{new_state.value()}};
        }

        if (auto new_state = StateArray::create_if_valid_start(c))
        {
            return Push{State{new_state.value()}};
        }

        if (auto new_state = StateObject::create_if_valid_start(c))
        {
            return Push{State{new_state.value()}};
        }

        if (auto new_state = StateNull::create_if_valid_start(c))
        {
            return Push{State{new_state.value()}};
        }

        throw std::runtime_error("Invalid JSON value");
    }

    StateOp operator()(StateNumber &state) const
    {
        switch (state.state)
        {
        case NO_DIGITS:
            if (this->c == '0')
            {
                state.state = ZERO;
            }
            else if (std::isdigit(this->c))
            {
                state.state = SOME_DIGIT;
            }
            else
            {
                throw std::runtime_error("Invalid character");
            }
            state.s.push_back(this->c);
            break;
        case SOME_DIGIT:
            if (std::isdigit(this->c))
            {
            }
            else if (this->c == '.')
            {
                state.state = DOT;
            }
            else if (this->c == 'e' || this->c == 'E')
            {
                state.state = EXP;
            }
            else
            {
                return Pop{JsonValue(std::strtod(state.s.c_str(), NULL))};
            }
            state.s.push_back(this->c);
            break;
        case ZERO:
            if (this->c == '.')
            {
                state.state = DOT;
            }
            else if (this->c == 'e' || this->c == 'E')
            {
                state.state = EXP;
            }
            else
            {
                return Pop{JsonValue(std::strtod(state.s.c_str(), NULL))};
            }
            state.s.push_back(this->c);
            break;
        case DOT:
            if (std::isdigit(this->c))
            {
                state.state = DOT_DIGITS;
            }
            else
            {
                throw std::runtime_error("Invalid character");
            }
            state.s.push_back(this->c);
            break;
        case DOT_DIGITS:
            if (std::isdigit(this->c))
            {
            }
            else if (this->c == 'e' || this->c == 'E')
            {
                state.state = EXP;
            }
            else
            {
                return Pop{JsonValue(std::strtod(state.s.c_str(), NULL))};
            }
            state.s.push_back(this->c);
            break;
        case EXP:
            if (std::isdigit(this->c))
            {
                state.state = EXP_DIGITS;
            }
            else if (this->c == 'e' || this->c == 'E')
            {
                state.state = EXP_SIGN;
            }
            else
            {
                throw std::runtime_error("Invalid character");
            }
            state.s.push_back(this->c);
            break;
        case EXP_SIGN:
            if (std::isdigit(this->c))
            {
                state.state = EXP_DIGITS;
            }
            else
            {
                throw std::runtime_error("Invalid character");
            }
            state.s.push_back(this->c);
            break;
        case EXP_DIGITS:
            if (std::isdigit(this->c))
            {
                state.state = EXP_DIGITS;
            }
            else
            {
                return Pop{JsonValue(std::strtod(state.s.c_str(), NULL))};
            }
            state.s.push_back(this->c);
            break;
        }

        return Noop{};
    }

    StateOp operator()(StateTrue &state) const
    {
        if (state.matched >= strlen(state.MATCH))
        {
            return Pop{JsonValue(true)};
        }

        ++state.matched;

        return Noop{};
    }

    StateOp operator()(StateFalse &state) const
    {
        if (state.matched >= strlen(state.MATCH))
        {
            return Pop{JsonValue(false)};
        }

        ++state.matched;

        return Noop{};
    }

    StateOp operator()(StateString &state) const
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
                return Pop{JsonValue(state.s), false};
            }
        }

        state.s.push_back(this->c);

        return Noop{};
    }

    StateOp operator()(const StateArray &state) const { return Noop{}; }

    StateOp operator()(const StateObject &state) const { return Noop{}; }

    StateOp operator()(StateNull &state) const
    {
        if (state.matched >= strlen(state.MATCH))
        {
            return Pop{JsonValue(nullptr)};
        }

        ++state.matched;

        return Noop{};
    }

    const char c;
    const size_t i;
};

struct StateTerminateVisitor
{
    JsonValue operator()(const StateValue &state) const
    {
        if (!state.value.has_value())
        {
            throw std::runtime_error("Unexpected end of input in JSON value");
        }
        return state.value.value();
    }

    JsonValue operator()(StateNumber &state) const
    {
        switch (state.state)
        {
        case ZERO:
        case SOME_DIGIT:
        case DOT_DIGITS:
        case EXP_DIGITS:
            return std::strtod(state.s.c_str(), NULL);
        default:
            throw std::runtime_error("Unexpected end of input in JSON number");
        }
    }

    JsonValue operator()(StateTrue &state) const
    {
        if (state.matched != strlen(state.MATCH))
        {
            throw std::runtime_error("Unexpected end of input in JSON true");
        }

        return JsonValue(true);
    }

    JsonValue operator()(const StateFalse &state) const
    {
        if (state.matched != strlen(state.MATCH))
        {
            throw std::runtime_error("Unexpected end of input in JSON false");
        }

        return JsonValue(false);
    }

    JsonValue operator()(const StateString &state) const
    {
        throw std::runtime_error("Unexpected end of input in JSON string");
    }

    JsonValue operator()(const StateArray &state) const
    {
        throw std::runtime_error("Unexpected end of input in JSON array");
    }

    JsonValue operator()(const StateObject &state) const
    {
        throw std::runtime_error("Unexpected end of input in JSON object");
    }

    JsonValue operator()(const StateNull &state) const
    {
        if (state.matched != strlen(state.MATCH))
        {
            throw std::runtime_error("Unexpected end of input in JSON null");
        }

        return JsonValue(nullptr);
    }
};

struct StatePopOpVisitor
{
    void operator()(StateValue &state)
    {
        state.value = this->value;
    }

    void operator()(StateArray &state) {}

    void operator()(StateObject &state) {}

    template <typename T>
    void operator()(T &)
    {
        throw std::runtime_error("Cannot handle popped state");
    }

    JsonValue value;
};

struct StateOpVisitor
{
    bool operator()(const Noop &op) const
    {
        return false;
    }

    bool operator()(const Push &op) const
    {
        states.push_back(op.state);
        return false;
    }

    bool operator()(const Pop &op) const
    {
        states.pop_back();

        if (states.size() < 1)
        {
            throw std::runtime_error("Extraneous input after JSON vlaue");
        }

        std::visit(StatePopOpVisitor{op.value}, states.back());

        return op.redo;
    }

    std::vector<State> &states;
};

JsonValue JsonValue::parse(const std::string_view json_str)
{
    auto it = json_str.begin();
    auto states = std::vector<State>{StateValue{std::nullopt}};

    for (size_t i = 0; i < json_str.size(); ++i)
    {
        auto c = json_str[i];
        while (1)
        {
            auto op = std::visit(StateCharVisitor{c, i}, states.back());
            auto redo = std::visit(StateOpVisitor{states}, op);
            if (!redo)
            {
                break;
            }
        }
    }

    while (states.size() > 1)
    {
        auto value = std::visit(StateTerminateVisitor{}, states.back());
        states.pop_back();
        std::visit(StatePopOpVisitor{value}, states.back());
    }

    if (states.size() > 1)
    {
        throw std::runtime_error("Incomplete JSON value provided");
    }

    return std::get<StateValue>(states.back()).value.value();
}