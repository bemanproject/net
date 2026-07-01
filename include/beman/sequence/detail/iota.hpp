// include/beman/sequence/detail/iota.hpp                             -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef INCLUDED_INCLUDE_BEMAN_SEQUENCE_DETAIL_IOTA
#define INCLUDED_INCLUDE_BEMAN_SEQUENCE_DETAIL_IOTA

#include <beman/sequence/detail/sequence_sender.hpp>
#include <beman/sequence/detail/set_next.hpp>
#include <type_traits>
#include <utility>

// ----------------------------------------------------------------------------

namespace beman::sequence::detail {
    struct iota_t {
        template <::beman::execution::receiver Rcvr, typename T>
        struct state {
            using operation_state_concept = ::beman::execution::operation_state_tag;
            ::std::remove_cvref_t<Rcvr> rcvr;
            T first;
            T last;

            void start() & noexcept {
                if (first != last) {
                    //::beman::sequence::set_next(this->rcvr, this->first);
                    ++this->first;
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
