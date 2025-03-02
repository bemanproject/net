// examples/demo_algorithm.hpp                                        -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef INCLUDED_EXAMPLES_DEMO_ALGORITHM
#define INCLUDED_EXAMPLES_DEMO_ALGORITHM

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-braces"
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#endif

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunknown-warning-option"
#pragma clang diagnostic ignored "-Wmissing-braces"
#endif

#include <beman/net/net.hpp>
#include <atomic>
#include <optional>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>
#include <version>
#if 202202L <= __cpp_lib_expected
#include <expected>
#endif

// ----------------------------------------------------------------------------

namespace demo {
namespace ex = ::beman::net::detail::ex;
}

namespace demo::detail {
template <typename, typename T>
struct into_error_transform_helper {
    using type = T;
};
template <typename Fun, typename... T>
struct into_error_transform_helper<Fun, ex::set_value_t(T...)> {
    using type = ex::set_error_t(decltype(::std::declval<Fun>()(::std::declval<T>()...)));
};
template <typename Fun>
struct into_error_transform {
    template <typename T>
    using type = typename into_error_transform_helper<Fun, T>::type;
};

template <typename>
struct variant_from_list;
template <typename... T>
struct variant_from_list<ex::detail::type_list<T...>> {
    using type = ex::detail::variant_or_empty<T...>;
};
template <typename... T>
struct variant_from_list<ex::completion_signatures<ex::set_error_t(T)...>> {
    using type = ex::detail::variant_or_empty<T...>;
};
template <typename L>
using variant_from_list_t = typename variant_from_list<L>::type;

template <typename T>
struct is_set_error {
    static constexpr bool value{false};
};
template <typename E>
struct is_set_error<ex::set_error_t(E)> {
    static constexpr bool value{true};
};

template <typename T>
struct is_set_value {
    static constexpr bool value{false};
};
template <typename... A>
struct is_set_value<ex::set_value_t(A...)> {
    static constexpr bool value{true};
};

template <typename T>
struct decayed_set_value;
template <typename... T>
struct decayed_set_value<ex::set_value_t(T...)> {
    using type = ::std::tuple<::std::decay_t<T>...>;
};
template <typename T>
using decayed_set_value_t = typename decayed_set_value<T>::type;

template <typename... T>
struct decayed_tuple_or_single {
    using type = ::std::tuple<::std::decay_t<T>...>;
};
template <typename T>
struct decayed_tuple_or_single<T> {
    using type = ::std::decay_t<T>;
};
template <typename... T>
using decayed_tuple_or_single_t = typename decayed_tuple_or_single<T...>::type;

template <typename>
struct make_type_list;
template <template <typename> class L, typename... T>
    requires(!::std::same_as<L<ex::set_stopped_t()>, ex::completion_signatures<ex::set_stopped_t()>>)
struct make_type_list<L<T...>> {
    using type = ex::detail::type_list<T...>;
};
template <typename... T>
struct make_type_list<ex::completion_signatures<T...>> {
    using type = ex::detail::type_list<T...>;
};
template <typename T>
using make_type_list_t = typename make_type_list<T>::type;
} // namespace demo::detail

namespace demo {
struct into_error_t {
    template <ex::receiver, typename>
    struct receiver;
    template <ex::sender, typename>
    struct sender;
    template <typename Fun>
    auto operator()(Fun&&) const;
    template <ex::sender Sender, typename Fun>
    auto operator()(Sender&&, Fun&&) const -> sender<::std::remove_cvref_t<Sender>, ::std::remove_cvref_t<Fun>>;
};
inline constexpr into_error_t into_error{};

#if 202202L <= __cpp_lib_expected
struct into_expected_t {
    auto operator()() const;
    template <ex::sender Sender>
    auto operator()(Sender&&) const;
};
inline constexpr into_expected_t into_expected{};
#endif

struct when_any_t {
    template <typename>
    struct env;
    template <typename>
    struct state_base;
    template <ex::receiver, typename, typename>
    struct state_value;
    template <::std::size_t, ex::receiver, typename, typename>
    struct receiver;

    template <typename, ex::receiver, typename, typename, ex::sender...>
    struct state;
    template <::std::size_t... I, ex::receiver Receiver, typename Value, typename Error, ex::sender... Sender>
    struct state<::std::index_sequence<I...>, Receiver, Value, Error, Sender...>;
    template <ex::sender...>
    struct sender;
    template <ex::sender... Sender>
        requires(0u < sizeof...(Sender))
    auto operator()(Sender&&...) const -> sender<Sender...>;
};
inline constexpr when_any_t when_any{};
} // namespace demo

// ----------------------------------------------------------------------------

template <demo::ex::receiver Receiver, typename Fun>
struct demo::into_error_t::receiver {
    using receiver_concept = ex::receiver_t;

    Receiver receiver;
    Fun      fun;
    auto     get_env() const noexcept { return ex::get_env(this->receiver); }
    template <typename E>
    auto set_error(E&& error) && noexcept -> void {
        ex::set_error(::std::move(this->receiver), ::std::forward<E>(error));
    }
    auto set_stopped() && noexcept -> void { ex::set_stopped(::std::move(this->receiver)); }
    template <typename... T>
    auto set_value(T&&... args) && noexcept -> void {
        ex::set_error(::std::move(this->receiver), ::std::move(this->fun)(::std::forward<T>(args)...));
    }
};

template <demo::ex::sender Sender, typename Fun>
struct demo::into_error_t::sender {
    using sender_concept = ex::sender_t;
    template <typename Env>
    auto get_completion_signatures(const Env& env) const {
        return ::beman::execution::detail::meta::transform<demo::detail::into_error_transform<Fun>::template type,
                                                           decltype(ex::get_completion_signatures(
                                                               ::std::declval<Sender>(), env))>();
    }

    template <ex::receiver Receiver>
    auto connect(Receiver&& receiver) && {
        return ex::connect(
            std::move(this->sender),
            demo::into_error_t::receiver<Receiver, Fun>{::std::forward<Receiver>(receiver), ::std::move(this->fun)});
    }

    Sender sender;
    Fun    fun;
};

template <demo::ex::sender Sender, typename Fun>
inline auto demo::into_error_t::operator()(Sender&& sndr, Fun&& fun) const
    -> demo::into_error_t::sender<::std::remove_cvref_t<Sender>, ::std::remove_cvref_t<Fun>> {
    return {::std::forward<Sender>(sndr), ::std::forward<Fun>(fun)};
}

template <typename Fun>
inline auto demo::into_error_t::operator()(Fun&& fun) const {
    return ex::detail::sender_adaptor{*this, fun};
}

// ----------------------------------------------------------------------------

#if 202202L <= __cpp_lib_expected
inline auto demo::into_expected_t::operator()() const { return beman::net::detail::ex::detail::sender_adaptor{*this}; }
template <beman::net::detail::ex::sender Sender>
inline auto demo::into_expected_t::operator()(Sender&& s) const {
    using value_type =
        ex::value_types_of_t<Sender, ex::empty_env, demo::detail::decayed_tuple_or_single_t, std::type_identity_t>;
    using error_type = ex::error_types_of_t<Sender>;
    return ::std::forward<Sender>(s) | beman::net::detail::ex::then([]<typename... A>(A&&... a) noexcept {
               return std::expected<value_type, error_type>(::std::in_place_t{}, ::std::forward<A>(a)...);
           }) |
           beman::net::detail::ex::upon_error([]<typename E>(E&& e) noexcept {
               return std::expected<value_type, error_type>(::std::unexpect_t{}, ::std::forward<E>(e));
           });
}
#endif

// ----------------------------------------------------------------------------

template <typename Receiver>
struct demo::when_any_t::state_base {
    ::std::size_t                   total{};
    Receiver                        receiver{};
    ::std::atomic<::std::size_t>    done_count{};
    ::std::atomic<::std::size_t>    ready_count{};
    ::demo::ex::inplace_stop_source source{};

    template <typename R>
    state_base(std::size_t tot, R&& rcvr) : total(tot), receiver(::std::forward<R>(rcvr)) {}
    virtual ~state_base() = default;
    auto complete() -> bool {
        if (0u == this->done_count++) {
            this->source.request_stop();
            return true;
        }
        return false;
    }
    auto virtual notify_done() -> void = 0;
    auto ready() -> void {
        if (++this->ready_count == this->total) {
            this->notify_done();
        }
    }
};

template <demo::ex::receiver Receiver, typename Value, typename Error>
struct demo::when_any_t::state_value : demo::when_any_t::state_base<Receiver> {
    ::std::optional<Error> error{};
    ::std::optional<Value> value{};

    template <typename R>
    state_value(::std::size_t tot, R&& rcvr) : state_base<Receiver>{tot, ::std::forward<R>(rcvr)} {}

    auto notify_done() -> void override {
        if (this->error) {
            ::demo::ex::set_error(::std::move(this->receiver), ::std::move(*this->error));
        } else if (this->value) {
            std::visit(
                [this](auto&& m) {
                    (void)this;
                    ::std::apply(
                        [this](auto&&... a) { ::demo::ex::set_value(::std::move(this->receiver), ::std::move(a)...); },
                        m);
                },
                *this->value);
        } else {
            ::demo::ex::set_stopped(::std::move(this->receiver));
        }
    }
};

// ----------------------------------------------------------------------------

template <typename Receiver>
struct demo::when_any_t::env {
    demo::when_any_t::state_base<Receiver>* state;
    auto query(const ex::get_stop_token_t&) const noexcept -> ex::inplace_stop_token {
        return this->state->source.get_token();
    }
    //-dk:TODO when_any_t::env: set up query forwarding
};

// ----------------------------------------------------------------------------

template <::std::size_t, ::demo::ex::receiver Receiver, typename Value, typename Error>
struct demo::when_any_t::receiver {
    using receiver_concept = ::demo::ex::receiver_t;
    demo::when_any_t::state_value<Receiver, Value, Error>* state;

    auto get_env() const noexcept -> env<Receiver> { return {this->state}; }
    template <typename E>
    auto set_error(E&& error) && noexcept -> void {
        if (this->state->complete()) {
            this->state->error.emplace(::std::forward<E>(error));
        }
        this->state->ready();
    }
    auto set_stopped() && noexcept -> void {
        this->state->complete();
        this->state->ready();
    }
    template <typename... A>
    auto set_value(A&&... a) && noexcept -> void {
        if (this->state->complete()) {
            this->state->value.emplace(
                Value(::std::in_place_type_t<::std::tuple<std::decay_t<A>...>>(), ::std::forward<A>(a)...));
        }
        this->state->ready();
    }
};

// ----------------------------------------------------------------------------

template <::std::size_t... I, demo::ex::receiver Receiver, typename Value, typename Error, demo::ex::sender... Sender>
struct demo::when_any_t::state<::std::index_sequence<I...>, Receiver, Value, Error, Sender...>
    : demo::when_any_t::state_value<Receiver, Value, Error> {
    using value_type = Value;
    using error_type = Error;

    template <::std::size_t J>
    using receiver_type           = when_any_t::receiver<J, Receiver, value_type, error_type>;
    using operation_state_concept = ex::operation_state_t;
    using states_type             = ::beman::execution::detail::product_type<decltype(demo::ex::connect(
        ::std::declval<Sender>(), ::std::declval<receiver_type<I>>()))...>;
    states_type states;

    template <typename R, typename P>
    state(R&& rcvr, P&& s)
        : state_value<Receiver, value_type, error_type>(sizeof...(Sender), ::std::forward<R>(rcvr)),
          states{demo::ex::connect(::beman::net::detail::ex::detail::forward_like<P>(s.template get<I>()),
                                   receiver_type<I>{this})...} {}
    state(state&&) = delete;
    auto start() & noexcept -> void { (demo::ex::start(this->states.template get<I>()), ...); }
};

// ----------------------------------------------------------------------------

template <demo::ex::sender... Sender>
struct demo::when_any_t::sender {
    ::beman::execution::detail::product_type<::std::remove_cvref_t<Sender>...> sender;
    using sender_concept        = ex::sender_t;
    using completion_signatures = ::beman::execution::detail::meta::unique<::beman::execution::detail::meta::combine<
        decltype(ex::get_completion_signatures(::std::declval<Sender&&>(), ex::empty_env{}))...>>;

    template <demo::ex::receiver Receiver>
    auto connect(Receiver&& receiver) && -> state<
        ::std::index_sequence_for<Sender...>,
        ::std::remove_cvref_t<Receiver>,
        demo::detail::variant_from_list_t<
            ex::detail::transform<demo::detail::decayed_set_value_t,
                                  demo::detail::make_type_list_t<ex::detail::filter<
                                      demo::detail::is_set_value,
                                      decltype(ex::get_completion_signatures(*this, ex::get_env(receiver)))>>>>,
        demo::detail::variant_from_list_t<
            ex::detail::filter<demo::detail::is_set_error,
                               decltype(ex::get_completion_signatures(*this, ex::get_env(receiver)))>>,
        Sender...> {
        return {::std::forward<Receiver>(receiver), ::std::move(this->sender)};
    }
};

template <demo::ex::sender... Sender>
    requires(0u < sizeof...(Sender))
inline auto demo::when_any_t::operator()(Sender&&... sndr) const -> ::demo::when_any_t::sender<Sender...> {
    return {::std::forward<Sender>(sndr)...};
}

// ----------------------------------------------------------------------------

#endif
