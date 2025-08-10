// include/beman/net/detail/io_context_scheduler.hpp                -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef INCLUDED_BEMAN_NET_DETAIL_IO_CONTEXT_SCHEDULER
#define INCLUDED_BEMAN_NET_DETAIL_IO_CONTEXT_SCHEDULER

// ----------------------------------------------------------------------------

#include <beman/net/detail/execution.hpp>
#include <beman/net/detail/context_base.hpp>
#include <cassert>

// ----------------------------------------------------------------------------

namespace beman::net::detail {
class io_context_scheduler;
}

// ----------------------------------------------------------------------------

class beman::net::detail::io_context_scheduler {
  private:
    ::beman::net::detail::context_base* d_context;

  public:
    using scheduler_concept = ::beman::net::detail::ex::scheduler_t;

    struct env {
        ::beman::net::detail::context_base* d_context;

        template <typename Signal>
        auto query(const ::beman::net::detail::ex::get_completion_scheduler_t<Signal>&) const noexcept
            -> io_context_scheduler {
            return this->d_context;
        }
    };
    struct sender {
        template <typename Receiver>
        struct state : ::beman::net::detail::context_base::task {
            using operation_state_concept = ::beman::net::detail::ex::operation_state_t;

            ::std::remove_cvref_t<Receiver>     d_receiver;
            ::beman::net::detail::context_base* d_context;

            state(Receiver&& receiver, ::beman::net::detail::context_base* context)
                : d_receiver(::std::forward<Receiver>(receiver)), d_context(context) {}

            auto start() & noexcept -> void { this->d_context->schedule(this); }
            auto complete() -> void override { ::beman::net::detail::ex::set_value(::std::move(this->d_receiver)); }
        };

        using sender_concept = ::beman::net::detail::ex::sender_t;
        ::beman::net::detail::context_base* d_context;

        template <typename Receiver>
        auto connect(Receiver&& receiver) -> state<Receiver> {
            return {::std::forward<Receiver>(receiver), this->d_context};
        }

        auto get_env() const noexcept -> env { return {this->d_context}; }
    };

    auto schedule() noexcept -> sender { return {this->d_context}; }
    auto operator==(const io_context_scheduler&) const -> bool = default;

    io_context_scheduler(::beman::net::detail::context_base* context) : d_context(context) { assert(this->d_context); }

    auto get_context() const { return this->d_context; }

    auto cancel(beman::net::detail::io_base* cancel_op, beman::net::detail::io_base* op) -> void {
        this->d_context->cancel(cancel_op, op);
    }
    auto accept(::beman::net::detail::context_base::accept_operation* op) -> ::beman::net::detail::submit_result {
        return this->d_context->accept(op);
    }
    auto connect(::beman::net::detail::context_base::connect_operation* op) -> ::beman::net::detail::submit_result {
        return this->d_context->connect(op);
    }
    auto receive(::beman::net::detail::context_base::receive_operation* op) -> ::beman::net::detail::submit_result {
        return this->d_context->receive(op);
    }
    auto send(::beman::net::detail::context_base::send_operation* op) -> ::beman::net::detail::submit_result {
        return this->d_context->send(op);
    }
    auto resume_at(::beman::net::detail::context_base::resume_at_operation* op)
        -> ::beman::net::detail::submit_result {
        return this->d_context->resume_at(op);
    }
};

static_assert(::beman::net::detail::ex::sender<beman::net::detail::io_context_scheduler::sender>);
static_assert(::beman::net::detail::ex::scheduler<beman::net::detail::io_context_scheduler>);

// ----------------------------------------------------------------------------

#endif
