// include/beman/net/detail/repeat_effect_until.hpp                   -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef INCLUDED_INCLUDE_BEMAN_NET_DETAIL_REPEAT_EFFECT_UNTIL
#define INCLUDED_INCLUDE_BEMAN_NET_DETAIL_REPEAT_EFFECT_UNTIL

#include <beman/execution/execution.hpp>
#include <optional>
#include <type_traits>
#include <utility>

// ----------------------------------------------------------------------------

namespace beman::net::detail {
struct repeat_effect_until_t : beman::execution::sender_adaptor_closure<repeat_effect_until_t> {
    template <beman::execution::sender Upstream, beman::execution::sender Body, typename Predicate>
    auto operator()(Upstream&& upstream, Body&& body, Predicate&& predicate) const {
        return sender<std::remove_cvref_t<Upstream>, std::remove_cvref_t<Body>, std::remove_cvref_t<Predicate>>{
            std::forward<Upstream>(upstream), std::forward<Body>(body), std::forward<Predicate>(predicate)};
    }

    template <beman::execution::sender Upstream,
              beman::execution::sender Body,
              typename Predicate,
              beman::execution::receiver Receiver>
    struct state {
        struct receiver {
            using receiver_concept = beman::execution::receiver_t;
            state* _state;
            auto   get_env() const noexcept -> beman::execution::env_of_t<Receiver>;
            auto   set_value() && noexcept -> void;
            template <typename Error>
            auto set_error(Error&& e) && noexcept -> void;
            auto set_stopped() && noexcept -> void;
        };
        using operation_state_concept = beman::execution::operation_state_t;
        using upstream_state          = beman::execution::connect_result_t<Upstream, receiver>;
        using body_state              = beman::execution::connect_result_t<Body, receiver>;
        struct connector {
            template <beman::execution::sender Sndr, beman::execution::receiver Rcvr>
            connector(Sndr&& sndr, Rcvr&& rcvr)
                : _state(beman::execution::connect(std::forward<Sndr>(sndr), std::forward<Rcvr>(rcvr))) {}
            connector(connector&&) = delete;
            body_state _state;
        };

        Body                     _body;
        Predicate                _predicate;
        Receiver                 _receiver;
        upstream_state           _up_state;
        std::optional<connector> _body_state{};

        template <beman::execution::sender Up,
                  beman::execution::sender By,
                  typename Pred,
                  beman::execution::receiver Rcvr>
        state(Up&& up, By&& by, Pred&& pred, Rcvr&& rcvr) noexcept
            : _body(std::forward<By>(by)),
              _predicate(std::forward<Pred>(pred)),
              _receiver(std::forward<Rcvr>(rcvr)),
              _up_state(beman::execution::connect(std::forward<Upstream>(up), receiver{this})) {}
        auto start() & noexcept -> void { beman::execution::start(_up_state); }
        auto run_next() & noexcept -> void {
            this->_body_state.reset();
            if (this->_predicate()) {
                beman::execution::set_value(std::move(this->_receiver));
            } else {
                this->_body_state.emplace(std::forward<Body>(this->_body), receiver{this});
                beman::execution::start(this->_body_state->_state);
            }
        }
    };
    template <beman::execution::sender Upstream, beman::execution::sender Body, typename Predicate>
    struct sender {
        using sender_concept = beman::execution::sender_t;
        using completion_signatures =
            beman::execution::completion_signatures<beman::execution::set_value_t(),
                                                    //-dk:TODO add error types of upstream and body
                                                    //-dk:TODO add stopped only if upstream or body can be stopped
                                                    beman::execution::set_stopped_t()>;

        Upstream  upstream;
        Body      body;
        Predicate predicate;

        template <beman::execution::receiver Receiver>
        auto connect(Receiver&& receiver) {
            return state<Upstream, Body, Predicate, std::remove_cvref_t<Receiver>>{std::move(this->upstream),
                                                                                   std::move(this->body),
                                                                                   std::move(this->predicate),
                                                                                   std::forward<Receiver>(receiver)};
        }
    };
};
} // namespace beman::net::detail

namespace beman::net {
using repeat_effect_until_t = beman::net::detail::repeat_effect_until_t;
inline constexpr repeat_effect_until_t repeat_effect_until{};
} // namespace beman::net

// ----------------------------------------------------------------------------

template <beman::execution::sender Upstream,
          beman::execution::sender Body,
          typename Predicate,
          beman::execution::receiver Receiver>
auto beman::net::detail::repeat_effect_until_t::state<Upstream, Body, Predicate, Receiver>::receiver::get_env()
    const noexcept -> beman::execution::env_of_t<Receiver> {
    return beman::execution::get_env(this->_state->_receiver);
}
template <beman::execution::sender Upstream,
          beman::execution::sender Body,
          typename Predicate,
          beman::execution::receiver Receiver>
auto beman::net::detail::repeat_effect_until_t::state<Upstream, Body, Predicate, Receiver>::receiver::
    set_value() && noexcept -> void {
    this->_state->run_next();
}
template <beman::execution::sender Upstream,
          beman::execution::sender Body,
          typename Predicate,
          beman::execution::receiver Receiver>
template <typename Error>
auto beman::net::detail::repeat_effect_until_t::state<Upstream, Body, Predicate, Receiver>::receiver::set_error(
    Error&& e) && noexcept -> void {
    beman::execution::set_error(std::move(this->_state->_receiver), std::forward<Error>(e));
}
template <beman::execution::sender Upstream,
          beman::execution::sender Body,
          typename Predicate,
          beman::execution::receiver Receiver>
auto beman::net::detail::repeat_effect_until_t::state<Upstream, Body, Predicate, Receiver>::receiver::
    set_stopped() && noexcept -> void {
    beman::execution::set_stopped(std::move(this->_state->_receiver));
}

// ----------------------------------------------------------------------------

#endif
