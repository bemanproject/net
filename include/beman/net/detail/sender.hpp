// include/beman/net/detail/sender.hpp                              -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef INCLUDED_BEMAN_NET_DETAIL_SENDER
#define INCLUDED_BEMAN_NET_DETAIL_SENDER

#include <beman/net/detail/io_base.hpp>
#include <beman/net/detail/execution.hpp>
#include <beman/net/detail/stop_token.hpp>
#include <atomic>
#include <optional>
#include <type_traits>
#include <utility>

// ----------------------------------------------------------------------------

namespace beman::net::detail {
template <::beman::net::detail::ex::receiver>
struct sender_state_base;
template <::beman::net::detail::ex::receiver>
struct sender_upstream_receiver;
template <typename, typename, ::beman::net::detail::ex::receiver, ::beman::net::detail::ex::sender>
struct sender_state;
template <typename, typename, ::beman::net::detail::ex::sender>
struct sender;
template <typename>
struct sender_cpo;
} // namespace beman::net::detail

// ----------------------------------------------------------------------------

template <::beman::net::detail::ex::receiver Receiver>
struct beman::net::detail::sender_state_base {
    Receiver           d_receiver;
    ::std::atomic<int> d_outstanding{};

    template <::beman::net::detail::ex::receiver R>
    sender_state_base(R&& r) : d_receiver(::std::forward<R>(r)) {}
    virtual ~sender_state_base()            = default;
    virtual auto start() & noexcept -> void = 0;
};

template <::beman::net::detail::ex::receiver Receiver>
struct beman::net::detail::sender_upstream_receiver {
    using receiver_concept = ::beman::net::detail::ex::receiver_t;
    ::beman::net::detail::sender_state_base<Receiver>* d_state;

    auto set_value() && noexcept -> void { this->d_state->start(); }
    template <typename Error>
    auto set_error(Error&& error) && noexcept -> void {
        ::beman::net::detail::ex::set_error(::std::move(this->d_state->d_receiver), ::std::forward<Error>(error));
    }
    auto set_stopped() && noexcept -> void {
        ::beman::net::detail::ex::set_stopped(::std::move(this->d_state->d_receiver));
    }
    auto get_env() const noexcept { return ::beman::net::detail::ex::get_env(this->d_state->d_receiver); }
};

template <typename Desc,
          typename Data,
          ::beman::net::detail::ex::receiver Receiver,
          ::beman::net::detail::ex::sender   UpstreamSender>
struct beman::net::detail::sender_state : Desc::operation, ::beman::net::detail::sender_state_base<Receiver> {
    using operation_state_concept = ::beman::net::detail::ex::operation_state_t;

    struct cancel_callback : ::beman::net::detail::io_base {
        sender_state* d_state;
        cancel_callback(sender_state* s)
            : ::beman::net::detail::io_base(::beman::net::detail::socket_id(), ::beman::net::event_type::none),
              d_state(s) {}
        cancel_callback(cancel_callback&&) = default;
        auto operator()() {
            if (1 < ++this->d_state->d_outstanding) {
                this->d_state->d_data.get_scheduler().cancel(this, this->d_state);
            }
        }
        auto complete() -> void override final {
            if (0u == --this->d_state->d_outstanding) {
                ::beman::net::detail::ex::set_stopped(::std::move(this->d_state->d_receiver));
            }
        }
        auto error(::std::error_code) -> void override final { this->complete(); }
        auto cancel() -> void override final { this->complete(); }
    };
    using upstream_state_t = decltype(::beman::net::detail::ex::connect(
        ::std::declval<UpstreamSender&>(), ::std::declval<sender_upstream_receiver<Receiver>>()));
    using stop_token       = decltype(::beman::net::detail::ex::get_stop_token(
        ::beman::net::detail::ex::get_env(::std::declval<const Receiver&>())));
    using callback         = typename stop_token::template callback_type<cancel_callback>;

    Data                      d_data;
    upstream_state_t          d_state;
    ::std::optional<callback> d_callback;

    template <typename D, ::beman::net::detail::ex::receiver R>
    sender_state(D&& d, R&& r, UpstreamSender up)
        : Desc::operation(d.id(), d.events()),
          sender_state_base<Receiver>(::std::forward<R>(r)),
          d_data(::std::forward<D>(d)),
          d_state(::beman::net::detail::ex::connect(up, sender_upstream_receiver<Receiver>{this})) {}
    auto start() & noexcept -> void override {
        auto token(::beman::net::detail::ex::get_stop_token(::beman::net::detail::ex::get_env(this->d_receiver)));
        static_assert(not std::same_as<ex::never_stop_token, void>);
        ++this->d_outstanding;
        this->d_callback.emplace(token, this);
        if (token.stop_requested()) {
            this->d_callback.reset();
            this->cancel();
            return;
        }
        if (this->d_data.submit(this) == ::beman::net::detail::submit_result::ready) {
            this->complete();
        }
    }
    auto complete() -> void override final {
        d_callback.reset();
        if (0 == --this->d_outstanding) {
            this->d_data.set_value(*this, ::std::move(this->d_receiver));
        }
    }
    auto error(::std::error_code err) -> void override final {
        d_callback.reset();
        if (0 == --this->d_outstanding) {
            ::beman::net::detail::ex::set_error(::std::move(this->d_receiver), std::move(err));
        }
    }
    auto cancel() -> void override final {
        if (0 == --this->d_outstanding) {
            ::beman::net::detail::ex::set_stopped(::std::move(this->d_receiver));
        }
    }
};

template <typename Desc, typename Data, ::beman::net::detail::ex::sender Upstream>
struct beman::net::detail::sender {
    using sender_concept = ::beman::net::detail::ex::sender_t;
    using completion_signatures =
        ::beman::net::detail::ex::completion_signatures<typename Data::completion_signature,
                                                        ::beman::net::detail::ex::set_error_t(::std::error_code),
                                                        ::beman::net::detail::ex::set_stopped_t()>;

    Data     d_data;
    Upstream d_upstream;

    template <::beman::net::detail::ex::receiver Receiver>
    auto connect(Receiver&& receiver) const& {
        return ::beman::net::detail::sender_state<Desc, Data, ::std::remove_cvref_t<Receiver>, Upstream>(
            this->d_data, ::std::forward<Receiver>(receiver), this->d_upstream);
    }
    template <::beman::net::detail::ex::receiver Receiver>
    auto connect(Receiver&& receiver) && {
        return ::beman::net::detail::sender_state<Desc, Data, ::std::remove_cvref_t<Receiver>, Upstream>(
            this->d_data, ::std::forward<Receiver>(receiver), this->d_upstream);
    }
};

template <typename Desc>
struct beman::net::detail::sender_cpo {
    template <typename Arg0, typename... Args>
        requires(!::beman::net::detail::ex::sender<::std::remove_cvref_t<Arg0>>) &&
                ::std::invocable<const sender_cpo, decltype(::beman::net::detail::ex::just()), Arg0, Args...>
    auto operator()(Arg0&& arg0, Args&&... args) const {
        return (*this)(::beman::net::detail::ex::just(), ::std::forward<Arg0>(arg0), ::std::forward<Args>(args)...);
    }
    template <::beman::net::detail::ex::sender Upstream, typename... Args>
    auto operator()(Upstream&& u, Args&&... args) const {
        using Data = Desc::template data<::std::decay_t<Args>...>;
        return ::beman::net::detail::sender<Desc, Data, ::std::remove_cvref_t<Upstream>>{
            Data{::std::forward<Args>(args)...}, ::std::forward<Upstream>(u)};
    }
};

// ----------------------------------------------------------------------------

#endif
