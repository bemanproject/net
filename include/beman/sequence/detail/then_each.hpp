// include/beman/sequence/detail/then_each.hpp                        -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef INCLUDED_INCLUDE_BEMAN_SEQUENCE_DETAIL_THEN_EACH
#define INCLUDED_INCLUDE_BEMAN_SEQUENCE_DETAIL_THEN_EACH

#include <beman/sequence/detail/sequence_sender.hpp>
#include <beman/sequence/detail/sequence_receiver.hpp>
#include <beman/sequence/detail/sequence_connect.hpp>
#include <beman/execution/execution.hpp>
#include <type_traits>
#include <utility>

// ----------------------------------------------------------------------------

namespace beman::sequence::detail {
struct then_each_t {
    template <beman::sequence::sequence_receiver Rcvr, typename Func>
    struct receiver {
        using receiver_concept = ::beman::sequence::sequence_receiver_tag;
        ::std::remove_cvref_t<Rcvr> rcvr;
        ::std::remove_cvref_t<Func> func;

        template <typename... A>
        void set_value(A&&... a) && noexcept {
            ::beman::execution::set_value(std::move(this->rcvr), std::forward<A>(a)...);
        }
        template <typename E>
        void set_error(E&& e) && noexcept {
            ::beman::execution::set_error(std::move(this->rcvr), std::forward<E>(e));
        }
        void set_stopped() && noexcept { ::beman::execution::set_stopped(std::move(this->rcvr)); }
        template <::beman::execution::sender Snd>
        auto set_next(Snd&& snd) noexcept {
            return std::forward<Snd>(snd) | ::beman::execution::then([this]<typename... A>(A&&... a) noexcept {
                       return this->func(std::forward<A>(a)...);
                   });
        }
    };
    template <::beman::sequence::sequence_receiver Rcvr, typename Func>
    receiver(Rcvr&&, Func&&) -> receiver<Rcvr, Func>;

    template <::beman::sequence::sequence_sender Sndr, typename Func>
    struct sender {
        using sender_concept = ::beman::sequence::sequence_sender_tag;
        ::std::remove_cvref_t<Sndr> sndr;
        ::std::remove_cvref_t<Func> func;

        template <typename, typename... E>
        static consteval auto get_completion_signatures() noexcept {
            return ::beman::execution::get_completion_signatures<Sndr, E...>();
        }

        template <::beman::execution::receiver Rcvr>
        auto sequence_connect(Rcvr&& rcvr) && noexcept {
            return ::beman::sequence::sequence_connect(::std::move(this->sndr),
                                                       receiver{::std::forward<Rcvr>(rcvr), ::std::move(this->func)});
        }
    };
    template <::beman::sequence::sequence_sender Sndr, typename Func>
    sender(Sndr&&, Func&&) -> sender<Sndr, Func>;

    template <::beman::sequence::sequence_sender Sndr, typename Func>
    auto operator()(Sndr&& sndr, Func&& func) const noexcept {
        return sender{::std::forward<Sndr>(sndr), ::std::forward<Func>(func)};
    }
};
} // namespace beman::sequence::detail

namespace beman::sequence {
using ::beman::sequence::detail::then_each_t;
inline constexpr ::beman::sequence::then_each_t then_each{};
} // namespace beman::sequence

// ----------------------------------------------------------------------------

#endif
