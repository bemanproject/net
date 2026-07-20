// include/beman/sequence/detail/filter_each.hpp                      -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef INCLUDED_INCLUDE_BEMAN_SEQUENCE_DETAIL_FILTER_EACH
#define INCLUDED_INCLUDE_BEMAN_SEQUENCE_DETAIL_FILTER_EACH

#include <beman/sequence/detail/sequence_connect.hpp>
#include <beman/sequence/detail/sequence_connect_result_t.hpp>
#include <beman/sequence/detail/sequence_receiver.hpp>
#include <beman/sequence/detail/sequence_sender.hpp>
#include <beman/sequence/detail/set_next.hpp>
#include <utility>

// ----------------------------------------------------------------------------

namespace beman::sequence::detail {
    struct filter_each_t {
        template <::beman::sequence::sequence_sender Sndr, typename Pred>
        struct sender {
            using sender_concept = sequence_sender_tag;
            template <typename...>
            static consteval auto get_completion_signatures() noexcept {
                return ::beman::execution::completion_signatures<::beman::execution::set_value_t()>();
            }

            Sndr sndr_;
            Pred pred_;

            template <::beman::sequence::sequence_receiver Rcvr>
            struct receiver {
                using receiver_concept = sequence_receiver_tag;
                Pred pred_;
                Rcvr rcvr_;

                template <::beman::execution::sender VSndr>
                auto set_next(VSndr&& vsndr) & noexcept {
                    return ::beman::sequence::set_next(this->rcvr_, ::std::forward<VSndr>(vsndr));
                }
                void set_value() && noexcept {
                    ::beman::execution::set_value(::std::move(this->rcvr_));
                }
                template <typename... E>
                void set_error(E&&... e) && noexcept {
                    ::beman::execution::set_error(::std::move(this->rcvr_), ::std::forward<E>(e)...);
                }
                void set_stopped() && noexcept {
                    ::beman::execution::set_stopped(::std::move(this->rcvr_));
                }
            };
            template <::beman::sequence::sequence_receiver Rcvr>
            struct state {
                using operation_state_concept = ::beman::execution::operation_state_tag;
                using inner_state_t = ::beman::sequence::sequence_connect_result_t<Sndr, receiver<Rcvr>>;

                inner_state_t inner_state_;

                template <::beman::sequence::sequence_sender S, typename P>
                state(S&& sndr, P&& pred, Rcvr&& rcvr)
                    : inner_state_(::beman::sequence::sequence_connect(::std::forward<S>(sndr), receiver<Rcvr>{::std::forward<P>(pred), ::std::forward<Rcvr>(rcvr)}))
                {
                }

                void start() & noexcept {
                    return ::beman::execution::start(this->inner_state_);
                }
            };

            template <::beman::sequence::sequence_receiver Rcvr>
            auto sequence_connect(Rcvr&& rcvr) && noexcept {
                return state<::std::remove_cvref_t<Rcvr>>{std::move(sndr_), std::move(pred_), std::forward<Rcvr>(rcvr)};
            }
        };
        template <::beman::sequence::sequence_sender Sndr, typename Pred>
        constexpr auto operator()(Sndr&& sndr, Pred&& pred) const noexcept {
            return sender<::std::remove_reference_t<Sndr>, ::std::remove_reference_t<Pred>>{::std::forward<Sndr>(sndr), ::std::forward<Pred>(pred)};
        }
    };
}

namespace beman::sequence {
    using detail::filter_each_t;
    inline constexpr detail::filter_each_t filter_each{};
}

// ----------------------------------------------------------------------------

#endif
