// examples/demo_task.hpp                                             -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef INCLUDED_EXAMPLES_DEMO_TASK
#define INCLUDED_EXAMPLES_DEMO_TASK

#include <beman/net/net.hpp>
#include <exception>
#include <coroutine>
#include <memory>
#include <tuple>
#include <optional>
#include <type_traits>

#if defined(__clang__)
#pragma clang diagnostic ignored "-Wreturn-type"
#endif

// ----------------------------------------------------------------------------

namespace demo {
namespace ex = ::beman::net::detail::ex;

template <typename T>
struct task_state_base {
    ::std::optional<T> task_result;
    virtual ~task_state_base()                                = default;
    virtual auto complete_value() -> void                     = 0;
    virtual auto complete_error(::std::exception_ptr) -> void = 0;
    virtual auto complete_stopped() -> void                   = 0;

    template <typename Receiver>
    auto complete_set_value(Receiver& receiver) {
        ::beman::net::detail::ex::set_value(::std::move(receiver), ::std::move(*this->task_result));
    }
};
template <>
struct task_state_base<void> {
    virtual ~task_state_base()                                = default;
    virtual auto complete_value() -> void                     = 0;
    virtual auto complete_error(::std::exception_ptr) -> void = 0;
    virtual auto complete_stopped() -> void                   = 0;

    template <typename Receiver>
    auto complete_set_value(Receiver& receiver) {
        ::beman::net::detail::ex::set_value(::std::move(receiver));
    }
};

struct task_none {};
template <typename... T>
struct task_type_or_none;
template <typename T>
struct task_type_or_none<T> {
    using type = T;
};
template <>
struct task_type_or_none<> {
    using type = task_none;
};
template <typename... T>
using task_type_or_none_t = typename task_type_or_none<T...>::type;

template <typename... T>
struct task_single_or_tuple {
    using type = ::std::tuple<::std::decay_t<T>...>;
};
template <typename T>
struct task_single_or_tuple<T> {
    using type = ::std::decay_t<T>;
};
template <typename... T>
using task_single_or_tuple_t = typename task_single_or_tuple<T...>::type;

template <typename R>
struct task_promise_result {
    task_state_base<R>* state{};
    template <typename T>
    auto return_value(T&& r) -> void {
        this->state->task_result.emplace(std::forward<T>(r));
    }
};
template <>
struct task_promise_result<void> {
    task_state_base<void>* state{};
    auto                   return_void() -> void {}
};

template <typename T>
struct task_completion {
    using type = ::beman::net::detail::ex::set_value_t(T);
};
template <>
struct task_completion<void> {
    using type = ::beman::net::detail::ex::set_value_t();
};

template <typename Result = void>
struct task {
    enum class stop_state { running, stopping, stopped };
    template <typename Promise, ex::sender Sender>
    struct sender_awaiter {
        struct env {
            sender_awaiter* awaiter{};
            auto            query(ex::get_stop_token_t) const noexcept -> ex::inplace_stop_token;
        };
        struct receiver {
            using receiver_concept = ex::receiver_t;

            sender_awaiter* awaiter{};

            template <typename... Args>
            auto set_value(Args&&... args) noexcept -> void {
                this->awaiter->result.emplace(::std::forward<Args>(args)...);
                this->awaiter->handle.resume();
            }
            template <typename Error>
            auto set_error(Error&& err) noexcept -> void {
                if constexpr (::std::same_as<::std::decay_t<Error>, ::std::exception_ptr>)
                    this->awaiter->error = err;
                else
                    this->awaiter->error = ::std::make_exception_ptr(::std::forward<Error>(err));
                this->awaiter->handle.resume();
            }
            auto set_stopped() noexcept -> void { this->awaiter->stop(); }
            auto get_env() const noexcept -> env { return {this->awaiter}; }
        };
        using value_type = ex::value_types_of_t<Sender, Promise, task_single_or_tuple_t, task_type_or_none_t>;
        using state_type = decltype(ex::connect(::std::declval<Sender>(), std::declval<receiver>()));

        ::std::coroutine_handle<Promise> handle;
        ::std::exception_ptr             error;
        ::std::optional<value_type>      result;
        state_type                       state;

        sender_awaiter(Sender sender) : state(ex::connect(::std::move(sender), receiver{this})) {}

        auto           stop() -> void;
        auto           get_token() const noexcept -> ex::inplace_stop_token;
        constexpr auto await_ready() const noexcept -> bool { return false; }
        auto           await_suspend(::std::coroutine_handle<Promise> hndle) -> void {
            this->handle = hndle;
            ex::start(this->state);
        }
        auto await_resume() {
            if (this->error)
                std::rethrow_exception(this->error);
            return ::std::move(*this->result);
        }
    };

    struct promise_type;
    struct final_awaiter {
        promise_type*  promise;
        constexpr auto await_ready() const noexcept -> bool { return false; }
        auto await_suspend(::std::coroutine_handle<>) noexcept -> void { this->promise->state->complete_value(); }
        constexpr auto await_resume() const noexcept -> void {}
    };

    struct promise_type : task_promise_result<Result> {
        task::stop_state        stop_state{task::stop_state::running};
        ex::inplace_stop_source stop_source{};

        auto initial_suspend() -> ::std::suspend_always { return {}; }
        auto final_suspend() noexcept -> final_awaiter { return {this}; }
        auto get_return_object() -> task { return {unique_handle(this)}; }
        auto unhandled_exception() -> void { this->state->complete_error(::std::current_exception()); }
        template <ex::sender Sender>
        auto await_transform(Sender&& sender) {
            return sender_awaiter<promise_type, ::std::remove_cvref_t<Sender>>(::std::forward<Sender>(sender));
        }
    };

    using deleter = decltype([](promise_type* p) { std::coroutine_handle<promise_type>::from_promise(*p).destroy(); });
    using unique_handle = std::unique_ptr<promise_type, deleter>;

    template <typename Receiver>
    struct state : task_state_base<::std::decay_t<Result>> {
        using operation_state_concept = ex::operation_state_t;
        struct callback_t {
            state* object;
            auto   operator()() const {
                auto state{this->object};
                state->callback.reset();
                state->handle->stop_state = task::stop_state::stopping;
                state->handle->stop_source.request_stop();
                if (state->handle->stop_state == task::stop_state::stopped)
                    this->object->handle->state->complete_stopped();
            }
        };
        using stop_token    = decltype(ex::get_stop_token(ex::get_env(::std::declval<Receiver>())));
        using stop_callback = ex::stop_callback_for_t<stop_token, callback_t>;

        unique_handle                  handle;
        ::std::decay_t<Receiver>       receiver;
        ::std::optional<stop_callback> callback;

        template <typename R>
        state(unique_handle hndl, R&& rcvr) : handle(::std::move(hndl)), receiver(::std::forward<R>(rcvr)) {}

        auto start() & noexcept -> void {
            this->handle->state = this;
            this->callback.emplace(ex::get_stop_token(ex::get_env(this->receiver)), callback_t{this});
            std::coroutine_handle<promise_type>::from_promise(*this->handle).resume();
        }

        auto complete_value() -> void override { this->complete_set_value(this->receiver); }
        auto complete_error(::std::exception_ptr error) -> void override {
            ::beman::net::detail::ex::set_error(::std::move(this->receiver), ::std::move(error));
        }
        auto complete_stopped() -> void override {
            ::beman::net::detail::ex::set_stopped(::std::move(this->receiver));
        }
    };

    unique_handle handle;

    using sender_concept = ::beman::net::detail::ex::sender_t;
    using completion_signatures =
        ::beman::net::detail::ex::completion_signatures<::beman::net::detail::ex::set_error_t(::std::exception_ptr),
                                                        ::beman::net::detail::ex::set_stopped_t(),
                                                        typename task_completion<::std::decay_t<Result>>::type>;

    template <typename Receiver>
    auto connect(Receiver&& receiver) {
        return state<Receiver>(::std::move(this->handle), ::std::forward<Receiver>(receiver));
    }
};
} // namespace demo

// ----------------------------------------------------------------------------

template <typename Result>
template <typename Promise, demo::ex::sender Sender>
auto demo::task<Result>::sender_awaiter<Promise, Sender>::env::query(demo::ex::get_stop_token_t) const noexcept
    -> demo::ex::inplace_stop_token {
    return this->awaiter->handle.promise().stop_source.get_token();
}

template <typename Result>
template <typename Promise, demo::ex::sender Sender>
auto demo::task<Result>::sender_awaiter<Promise, Sender>::stop() -> void {
    if (::std::exchange(this->handle.promise().stop_state, task::stop_state::stopped) == task::stop_state::running) {
        this->handle.promise().state->complete_stopped();
    }
}

// ----------------------------------------------------------------------------

#endif
