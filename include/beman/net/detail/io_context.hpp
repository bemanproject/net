// include/beman/net/detail/io_context.hpp                          -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef INCLUDED_BEMAN_NET_DETAIL_IO_CONTEXT
#define INCLUDED_BEMAN_NET_DETAIL_IO_CONTEXT

// ----------------------------------------------------------------------------

#include <beman/net/detail/netfwd.hpp>
#include <beman/net/detail/container.hpp>
#include <beman/net/detail/context_base.hpp>
#include <beman/net/detail/io_context_scheduler.hpp>
#include <beman/net/detail/poll_context.hpp>
#include <beman/net/detail/repeat_effect_until.hpp>
#include <beman/execution/execution.hpp>

#include <cstdint>
#include <sys/socket.h>
#include <unistd.h>
#include <poll.h>
#include <cerrno>
#include <csignal>
#include <limits>
#include <system_error>

// ----------------------------------------------------------------------------

namespace beman::net {
class io_context;
}

// ----------------------------------------------------------------------------

class beman::net::io_context {
  private:
    ::std::unique_ptr<::beman::net::detail::context_base> d_owned{new ::beman::net::detail::poll_context()};
    ::beman::net::detail::context_base&                   d_context{*this->d_owned};

  public:
    using scheduler_type = ::beman::net::detail::io_context_scheduler;
    class executor_type {};

    class handle {
      public:
        explicit handle(beman::net::io_context* ctxt) : context(ctxt) {}
        auto get_io_context() const -> beman::net::io_context& { return *this->context; }

      private:
        beman::net::io_context* context{};
    };
    auto get_handle() -> handle { return handle(this); }

    io_context() { std::signal(SIGPIPE, SIG_IGN); }
    io_context(::beman::net::detail::context_base& context) : d_owned(), d_context(context) {}
    io_context(io_context&&) = delete;

    auto make_socket(int d, int t, int p, ::std::error_code& error) -> ::beman::net::detail::socket_id {
        return this->d_context.make_socket(d, t, p, error);
    }
    auto release(::beman::net::detail::socket_id id, ::std::error_code& error) -> void {
        return this->d_context.release(id, error);
    }
    auto native_handle(::beman::net::detail::socket_id id) -> ::beman::net::detail::native_handle_type {
        return this->d_context.native_handle(id);
    }
    auto set_option(::beman::net::detail::socket_id id,
                    int                             level,
                    int                             name,
                    const void*                     data,
                    ::socklen_t                     size,
                    ::std::error_code&              error) -> void {
        this->d_context.set_option(id, level, name, data, size, error);
    }
    auto bind(::beman::net::detail::socket_id                                id,
              const ::beman::net::ip::basic_endpoint<::beman::net::ip::tcp>& endpoint,
              ::std::error_code&                                             error) {
        this->d_context.bind(id, ::beman::net::detail::endpoint(endpoint), error);
    }
    auto listen(::beman::net::detail::socket_id id, int no, ::std::error_code& error) {
        this->d_context.listen(id, no, error);
    }
    auto get_scheduler() -> scheduler_type { return scheduler_type(&this->d_context); }

    template <beman::execution::receiver Receiver>
    struct run_one_state {
        using operation_state_concept = ::beman::execution::operation_state_t;

        beman::net::io_context*         _context;
        ::std::remove_cvref_t<Receiver> _receiver;

        run_one_state(beman::net::io_context* context, Receiver&& receiver) noexcept
            : _context(context), _receiver(::std::forward<Receiver>(receiver)) {}
        run_one_state(run_one_state&&) = delete;
        auto start() & noexcept -> void {
            try {
                ::beman::execution::set_value(::std::move(this->_receiver), this->_context->run_one());
            } catch (...) {
                //-dk:TODO deal with exceptions in async_run_one
                std::cout << "run_one_state exception caught\n";
            }
        }
    };

    struct run_one_sender {
        using sender_concept = ::beman::execution::sender_t;
        using completion_signatures =
            ::beman::execution::completion_signatures<::beman::execution::set_value_t(std::size_t),
                                                      ::beman::execution::set_stopped_t()>;

        beman::net::io_context* _context;
        template <beman::execution::receiver Receiver>
        auto connect(Receiver&& receiver) {
            return run_one_state<Receiver>(this->_context, ::std::forward<Receiver>(receiver));
        }
    };

    auto async_run_one() { return run_one_sender{this}; }
    auto async_run() {
        return beman::execution::let_value(beman::execution::just(), [this, last_count = std::size_t(1)]() mutable {
            (void)last_count; //-dk:TODO remove this once no compiler complains about last_count being unused
            return beman::net::repeat_effect_until(
                beman::execution::just(),
                [this] { return this->async_run_one(); }() |
                    beman::execution::then([&last_count](std::size_t count) { last_count = count; }),
                [&last_count] { return last_count == 0; });
        });
    }
    ::std::size_t run_one() { return this->d_context.run_one(); }
    ::std::size_t run() {
        ::std::size_t count{};
        while (::std::size_t c = this->run_one()) {
            count += c;
        }
        return count;
    }
};

// ----------------------------------------------------------------------------

#endif
