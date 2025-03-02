// include/beman/net/detail/context_base.hpp                        -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef INCLUDED_BEMAN_NET_DETAIL_CONTEXT_BASE
#define INCLUDED_BEMAN_NET_DETAIL_CONTEXT_BASE

#include <beman/net/detail/io_base.hpp>
#include <beman/net/detail/endpoint.hpp>
#include <chrono>
#include <optional>
#include <system_error>
#include <sys/socket.h>
#include <sys/time.h>

// ----------------------------------------------------------------------------

namespace beman::net::detail {
struct context_base;
}

// ----------------------------------------------------------------------------

struct beman::net::detail::context_base {
    struct task {
        task* next;
        virtual ~task()                 = default;
        virtual auto complete() -> void = 0;
    };

    using accept_operation = ::beman::net::detail::io_operation<
        ::std::tuple<::beman::net::detail::endpoint, ::socklen_t, ::std::optional<::beman::net::detail::socket_id>>>;
    using connect_operation = ::beman::net::detail::io_operation<::std::tuple<::beman::net::detail::endpoint>>;
    using receive_operation = ::beman::net::detail::io_operation<::std::tuple<::msghdr, int, ::std::size_t>>;
    using send_operation    = ::beman::net::detail::io_operation<::std::tuple<::msghdr, int, ::std::size_t>>;
    using resume_after_operation =
        ::beman::net::detail::io_operation<::std::tuple<::std::chrono::system_clock::time_point, ::timeval>>;
    using resume_at_operation =
        ::beman::net::detail::io_operation<::std::tuple<::std::chrono::system_clock::time_point, ::timeval>>;

    virtual ~context_base()                                                                                 = default;
    virtual auto make_socket(int) -> ::beman::net::detail::socket_id                                        = 0;
    virtual auto make_socket(int, int, int, ::std::error_code&) -> ::beman::net::detail::socket_id          = 0;
    virtual auto release(::beman::net::detail::socket_id, ::std::error_code&) -> void                       = 0;
    virtual auto native_handle(::beman::net::detail::socket_id) -> ::beman::net::detail::native_handle_type = 0;
    virtual auto set_option(::beman::net::detail::socket_id, int, int, const void*, ::socklen_t, ::std::error_code&)
        -> void = 0;
    virtual auto bind(::beman::net::detail::socket_id, const ::beman::net::detail::endpoint&, ::std::error_code&)
        -> void                                                                           = 0;
    virtual auto listen(::beman::net::detail::socket_id, int, ::std::error_code&) -> void = 0;

    virtual auto run_one() -> ::std::size_t = 0;

    virtual auto cancel(::beman::net::detail::io_base*, ::beman::net::detail::io_base*) -> void = 0;
    virtual auto schedule(::beman::net::detail::context_base::task*) -> void                    = 0;
    virtual auto accept(::beman::net::detail::context_base::accept_operation*)
        -> ::beman::net::detail::submit_result = 0;
    virtual auto connect(::beman::net::detail::context_base::connect_operation*)
        -> ::beman::net::detail::submit_result = 0;
    virtual auto receive(::beman::net::detail::context_base::receive_operation*)
        -> ::beman::net::detail::submit_result                                                                    = 0;
    virtual auto send(::beman::net::detail::context_base::send_operation*) -> ::beman::net::detail::submit_result = 0;
    virtual auto resume_at(::beman::net::detail::context_base::resume_at_operation*)
        -> ::beman::net::detail::submit_result = 0;
};

// ----------------------------------------------------------------------------

#endif
