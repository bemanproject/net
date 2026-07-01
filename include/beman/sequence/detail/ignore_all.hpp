// include/beman/sequence/detail/ignore_all.hpp                       -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef INCLUDED_INCLUDE_BEMAN_SEQUENCE_DETAIL_IGNORE_ALL
#define INCLUDED_INCLUDE_BEMAN_SEQUENCE_DETAIL_IGNORE_ALL

#include <beman/sequence/detail/sequence_sender.hpp>
#include <beman/execution/execution.hpp>

// ----------------------------------------------------------------------------

namespace beman::sequence::detail {
    struct ignore_all_t {
        template <::beman::execution::receiver Rcvr, ::beman::sequence::sequence_sender Sndr>
        struct state {
            using operation_state_concept = ::beman::execution::operation_state_tag;
            struct receiver {
                using receiver_concept = ::beman::execution::receiver_tag;
                state* st;

                void set_value() && noexcept {
                    ::beman::execution::set_value(::std::move(this->st->rcvr));
                }
                template <::beman::execution::sender Snd>
                auto set_next(Snd&& snd) & noexcept {
                    return ::std::forward<Snd>(snd);
                }
            };
            using inner_state_t = ::beman::execution::connect_result_t<Sndr, receiver>;

            ::std::remove_cvref_t<Rcvr> rcvr;
            inner_state_t inner_state;

            state(Rcvr&& rcvr, Sndr&& sndr) noexcept
                : rcvr(::std::forward<Rcvr>(rcvr))
                , inner_state(::beman::execution::connect(::std::forward<Sndr>(sndr), receiver{this})) {}
            void start() & noexcept {
                ::beman::execution::start(this->inner_state);
            }
        };

        template <::beman::sequence::sequence_sender Sndr>
        struct sender {
            using sender_concept = ::beman::sequence::sequence_sender_tag;
            using sndr_t = ::std::remove_cvref_t<Sndr>;
            template <typename, typename... E>
            static consteval auto get_completion_signatures() noexcept {
                return ::beman::execution::get_completion_signatures<::std::remove_cvref_t<Sndr>, E...>();
            }

            sndr_t sndr;

            template <::beman::execution::receiver Rcvr>
            auto connect(Rcvr&& rcvr) && noexcept {
                return state<Rcvr, sndr_t>{::std::forward<Rcvr>(rcvr), ::std::move(this->sndr)};
            }
            template <::beman::execution::receiver Rcvr>
            auto connect(Rcvr&& rcvr) const& noexcept {
                return state<Rcvr, sndr_t const&>{::std::forward<Rcvr>(rcvr), this->sndr};
            }
        };

        template <::beman::sequence::sequence_sender Sndr>
        constexpr auto operator()(Sndr&& sndr) const noexcept {
            return sender<Sndr>{::std::forward<Sndr>(sndr)};
        }
    };
}

namespace beman::sequence {
    using ignore_all_t = ::beman::sequence::detail::ignore_all_t;
    inline constexpr ignore_all_t ignore_all{};
}

// ----------------------------------------------------------------------------

#endif
