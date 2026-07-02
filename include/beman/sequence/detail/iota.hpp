// include/beman/sequence/detail/iota.hpp                             -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef INCLUDED_INCLUDE_BEMAN_SEQUENCE_DETAIL_IOTA
#define INCLUDED_INCLUDE_BEMAN_SEQUENCE_DETAIL_IOTA

#include <beman/sequence/detail/sequence_sender.hpp>
#include <beman/sequence/detail/connector.hpp>
#include <beman/sequence/detail/set_next.hpp>
#include <type_traits>
#include <utility>

// ----------------------------------------------------------------------------

namespace beman::sequence::detail {
    struct iota_t {
        template <::beman::execution::receiver Rcvr, typename T>
        struct state {
            using operation_state_concept = ::beman::execution::operation_state_tag;
            using rcvr_t = ::std::remove_cvref_t<Rcvr>;
            using sndr_t = decltype(::beman::sequence::set_next(std::declval<rcvr_t&>(), ::beman::execution::just(std::declval<T>())));

            rcvr_t rcvr;
            struct receiver {
                using receiver_concept = ::beman::execution::receiver_tag;
                state* st;
                auto get_env() const noexcept { return ::beman::execution::get_env(this->st->rcvr); }
                auto set_value() && noexcept { this->st->start_next(); }
            };
            T first;
            T last;

            std::optional<::beman::sequence::detail::connector<sndr_t, receiver>> inner_state;

            void start() & noexcept {
                this->start_next();
            }
            void start_next() noexcept {
                if (first != last) {
                    ::beman::execution::start(
                        this->inner_state.emplace(
                            ::beman::sequence::set_next(this->rcvr, ::beman::execution::just(this->first++)),
                            receiver{this}
                        )
                    );
                } else {
                    ::beman::execution::set_value(::std::move(this->rcvr));
                }
            }
        };
        template <typename T>
        struct sender {
            using sender_concept = ::beman::sequence::sequence_sender_tag;
            template <typename, typename...>
            static consteval auto get_completion_signatures() noexcept {
                return ::beman::execution::completion_signatures<::beman::execution::set_value_t()>();
            }

            T first;
            T last;

            template <::beman::execution::receiver Rcvr>
            auto connect(Rcvr&& rcvr) && noexcept {
                static_assert(::beman::execution::operation_state<state<Rcvr, T>>);
                return state<Rcvr, T>{::std::forward<Rcvr>(rcvr), ::std::move(first), ::std::move(last)};
            }
            template <::beman::execution::receiver Rcvr>
            auto connect(Rcvr&& rcvr) const& noexcept {
                static_assert(::beman::execution::operation_state<state<Rcvr, T>>);
                return state<Rcvr, T>{::std::forward<Rcvr>(rcvr), first, last};
            }
        };

        template <typename T>
        constexpr auto operator()(T first, T last) const {
            return sender<T>{std::move(first), std::move(last)};
        }
    };
}

namespace beman::sequence {
    using iota_t = ::beman::sequence::detail::iota_t;
    inline constexpr iota_t iota{};
}

// ----------------------------------------------------------------------------

#endif
