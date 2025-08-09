// include/beman/net/detail/into_expected.hpp                         -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef INCLUDED_INCLUDE_BEMAN_NET_DETAIL_INTO_EXPECTED
#define INCLUDED_INCLUDE_BEMAN_NET_DETAIL_INTO_EXPECTED

#include <beman/execution/execution.hpp>
#include <concepts>
#include <expected>
#include <type_traits>

// ----------------------------------------------------------------------------

namespace beman::net::detail {
struct into_expected_t : beman::execution::sender_adaptor_closure<into_expected_t> {
    template <typename Sender, typename Env>
    using expected_t = std::expected<beman::execution::value_types_of_t<Sender, Env, std::tuple, std::type_identity_t>,
                                     beman::execution::error_types_of_t<Sender, Env, std::type_identity_t>>;

    template <beman::execution::sender Sender, beman::execution::receiver Receiver>
    struct state {
        struct receiver {
            using receiver_concept = beman::execution::receiver_t;
            using env_t            = beman::execution::env_of_t<Receiver>;
            Receiver* _receiver;
            auto      get_env() const noexcept { return beman::execution::get_env(*this->_receiver); }
            template <typename... Args>
            auto set_value(Args&&... args) && noexcept {
                std::cout << "into_expected: success\n";
                beman::execution::set_value(std::move(*this->_receiver),
                                            expected_t<Sender, env_t>(std::forward<Args>(args)...));
            }
            template <typename Error>
            auto set_error(Error&& error) && noexcept {
                std::cout << "into_expected: failure\n";
                beman::execution::set_value(std::move(*this->_receiver),
                                            expected_t<Sender, env_t>(std::unexpected(std::forward<Error>(error))));
            }
            auto set_stopped() && noexcept {
                std::cout << "into_expected: cancel\n";
                beman::execution::set_stopped(std::move(*this->_receiver));
            }
        };
        using operation_state_concept = beman::execution::operation_state_t;
        using inner_state_t           = beman::execution::connect_result_t<Sender, receiver>;

        Receiver      _receiver;
        inner_state_t _state;

        state(auto&& sndr, Receiver&& r)
            : _receiver(std::forward<Receiver>(r)),
              _state(beman::execution::connect(std::forward<Sender>(sndr), receiver{&this->_receiver})) {}

        auto start() & noexcept -> void { beman::execution::start(this->_state); }
    };
    template <beman::execution::sender Sender>
    struct sender {
        using sender_concept = beman::execution::sender_t;

        Sender _sender;

        template <typename Env>
        auto get_completion_signatures(const Env&) const {
            return beman::execution::completion_signatures<beman::execution::set_value_t(expected_t<Sender, Env>),
                                                           beman::execution::set_stopped_t()>();
        }
        template <beman::execution::receiver Receiver>
        auto connect(Receiver&& receiver) {
            return state<Sender, std::remove_cvref_t<Receiver>>(std::move(_sender), std::forward<Receiver>(receiver));
        }
    };

    template <beman::execution::sender Sender>
    auto operator()(Sender&& sndr) const {
        return sender<std::remove_cvref_t<Sender>>{std::forward<Sender>(sndr)};
    }
};

inline constexpr into_expected_t into_expected{};
} // namespace beman::net::detail

namespace beman::net {
using into_expected_t = beman::net::detail::into_expected_t;
inline constexpr into_expected_t into_expected{};
} // namespace beman::net

// ----------------------------------------------------------------------------

#endif
