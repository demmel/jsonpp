#pragma once

#include <functional>
#include <string>
#include <optional>
#include <variant>
#include <vector>

#include "utils.hpp"

namespace jsonpp::pda
{

    struct Noop
    {
    };

    template <typename TState>
    struct Push
    {
        TState state;
        bool redo = false;
    };

    struct Pop
    {
        bool redo = true;
    };

    struct PopOrAccept
    {
    };

    struct Accept
    {
    };

    struct Reject
    {
        std::string reason;
    };

    template <typename TState>
    using StateOp = std::variant<Noop, Push<TState>, Pop, Accept, Reject>;

    using FinalizeOp = std::variant<PopOrAccept, Accept, Reject>;

    struct PoppedEmptyError
    {
    };

    struct RejectedError
    {
        std::string reason;
    };

    using TransitionError = std::variant<PoppedEmptyError, RejectedError>;

    template <typename TState>
    using TransitionResult = std::variant<std::optional<TState>, TransitionError>;

    using FinalizeError = std::variant<RejectedError>;

    template <typename TState>
    using FinalizeResult = std::variant<TState, FinalizeError>;

    template <typename TState, typename TInput>
    class PushdownAutomata
    {
    public:
        PushdownAutomata(TState initial) : stack(1, initial) {}
        PushdownAutomata() = delete;

        TransitionResult<TState> transition(
            TInput input,
            std::function<StateOp<TState>(TState &, TInput)> handle_transition,
            std::function<std::optional<Reject>(TState &, TState &)> on_pop_impl);
        FinalizeResult<TState> finalize(
            std::function<FinalizeOp(TState &)> handle_finalize,
            std::function<std::optional<Reject>(TState &, TState &)> handle_pop);

    private:
        std::vector<TState> stack;
    };

    template <typename TState, typename TInput>
    TransitionResult<TState> PushdownAutomata<TState, TInput>::transition(
        TInput input,
        std::function<StateOp<TState>(TState &, TInput)> handle_transition,
        std::function<std::optional<Reject>(TState &, TState &)> on_pop_impl)
    {
        while (1)
        {
            auto op = handle_transition(this->stack.back(), input);
            using Result = std::variant<TState, bool, TransitionError>;
            Result res = std::visit(
                utils::inline_visitor{
                    [](const Noop &) -> Result
                    {
                        return false;
                    },
                    [this](const Push<TState> &op) -> Result
                    {
                        this->stack.push_back(op.state);
                        return op.redo;
                    },
                    [this, on_pop_impl](const Pop &op) -> Result
                    {
                        if (this->stack.size() < 2)
                        {
                            return PoppedEmptyError{};
                        }

                        auto popped = this->stack.back();
                        this->stack.pop_back();

                        if (auto rejection = on_pop_impl(this->stack.back(), popped))
                        {
                            return RejectedError{rejection.value().reason};
                        }

                        return op.redo;
                    },
                    [this](const Accept &) -> Result
                    {
                        return this->stack.back();
                    },
                    [](const Reject &reject) -> Result
                    {
                        return RejectedError{reject.reason};
                    },
                },
                op);

            auto accepted = std::get_if<TState>(&res);
            if (accepted)
            {
                return *accepted;
            }

            auto error = std::get_if<TransitionError>(&res);
            if (error)
            {
                return *error;
            }

            auto redo = std::get<bool>(res);
            if (!redo)
            {
                break;
            }
        }

        return std::nullopt;
    }

    template <typename TState, typename TInput>
    FinalizeResult<TState> PushdownAutomata<TState, TInput>::finalize(
        std::function<FinalizeOp(TState &)> handle_finalize,
        std::function<std::optional<Reject>(TState &, TState &)> on_pop_impl)
    {
        while (this->stack.size() > 1)
        {
            auto res = handle_finalize(this->stack.back());

            if (auto accept = std::get_if<Accept>(&res))
            {
                return this->stack.back();
            }
            else if (auto reject = std::get_if<Reject>(&res))
            {
                return RejectedError{reject->reason};
            }
            else if (auto pop_or_accept = std::get_if<PopOrAccept>(&res))
            {
                auto popped = this->stack.back();
                this->stack.pop_back();
                if (auto rejection = on_pop_impl(this->stack.back(), popped))
                {
                    return RejectedError{rejection.value().reason};
                }
                if (this->stack.size() < 1)
                {
                    return popped;
                }
            }
        }

        auto res = handle_finalize(this->stack.back());
        if (auto accept = std::get_if<Accept>(&res))
        {
            return this->stack.back();
        }
        else if (auto reject = std::get_if<Reject>(&res))
        {
            return RejectedError{reject->reason};
        }

        return this->stack.back();
    }
}