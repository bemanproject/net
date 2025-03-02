// include/beman/net/detail/timer.hpp                               -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef INCLUDED_BEMAN_NET_DETAIL_TIMER
#define INCLUDED_BEMAN_NET_DETAIL_TIMER

// ----------------------------------------------------------------------------

#include <beman/net/detail/netfwd.hpp>
#include <beman/net/detail/sender.hpp>
#include <beman/net/detail/event_type.hpp>

// ----------------------------------------------------------------------------

namespace beman::net::detail {
struct resume_after_desc;
struct resume_at_desc;
} // namespace beman::net::detail

namespace beman::net {
using async_resume_after_t = ::beman::net::detail::sender_cpo<::beman::net::detail::resume_after_desc>;
using async_resume_at_t    = ::beman::net::detail::sender_cpo<::beman::net::detail::resume_at_desc>;

inline constexpr async_resume_after_t resume_after{};
inline constexpr async_resume_at_t    resume_at{};
} // namespace beman::net

// ----------------------------------------------------------------------------

struct beman::net::detail::resume_after_desc {
    using operation = ::beman::net::detail::context_base::resume_after_operation;
    template <typename Scheduler, typename>
    struct data {
        using completion_signature = ::beman::net::detail::ex::set_value_t();

        ::std::remove_cvref_t<Scheduler> d_scheduler;
        ::std::chrono::microseconds      d_duration;

        auto id() const -> ::beman::net::detail::socket_id { return {}; }
        auto events() const { return ::beman::net::event_type::none; }
        auto get_scheduler() { return this->d_scheduler; }
        auto set_value(operation&, auto&& receiver) { ::beman::net::detail::ex::set_value(::std::move(receiver)); }
        auto submit(auto* base) -> ::beman::net::detail::submit_result {
            ::std::get<0>(*base) = ::std::chrono::system_clock::now() + this->d_duration;
            return this->d_scheduler.resume_at(base);
        }
    };
};

// ----------------------------------------------------------------------------

struct beman::net::detail::resume_at_desc {
    using operation = ::beman::net::detail::context_base::resume_at_operation;
    template <typename Scheduler, typename>
    struct data {
        using completion_signature = ::beman::net::detail::ex::set_value_t();

        ::std::remove_cvref_t<Scheduler>        d_scheduler;
        ::std::chrono::system_clock::time_point d_time;

        auto id() const -> ::beman::net::detail::socket_id { return {}; }
        auto events() const { return ::beman::net::event_type::none; }
        auto get_scheduler() { return this->d_scheduler; }
        auto set_value(operation&, auto&& receiver) { ::beman::net::detail::ex::set_value(::std::move(receiver)); }
        auto submit(auto* base) -> bool {
            ::std::get<0>(*base) = this->d_time;
            return this->d_scheduler.resume_at(base);
        }
    };
};

// ----------------------------------------------------------------------------

#endif
