// include/beman/net/detail/basic_socket.hpp                        -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef INCLUDED_BEMAN_NET_DETAIL_BASIC_SOCKET
#define INCLUDED_BEMAN_NET_DETAIL_BASIC_SOCKET

// ----------------------------------------------------------------------------

#include <beman/net/detail/netfwd.hpp>
#include <beman/net/detail/socket_base.hpp>
#include <beman/net/detail/io_context_scheduler.hpp>
#include <beman/net/detail/internet.hpp>

// ----------------------------------------------------------------------------

template <typename Protocol>
class beman::net::basic_socket : public ::beman::net::socket_base {
  public:
    using scheduler_type = ::beman::net::detail::io_context_scheduler;
    using protocol_type  = Protocol;

  private:
    ::beman::net::detail::context_base* d_context;
    protocol_type                       d_protocol{::beman::net::ip::tcp::v6()};
    ::beman::net::detail::socket_id     d_id{::beman::net::detail::socket_id::invalid};

  public:
    basic_socket() : d_context(nullptr) {}
    basic_socket(::beman::net::detail::context_base* context, ::beman::net::detail::socket_id id)
        : d_context(context), d_id(id) {}
    basic_socket(basic_socket&& other)
        : d_context(other.d_context),
          d_protocol(other.d_protocol),
          d_id(::std::exchange(other.d_id, ::beman::net::detail::socket_id::invalid)) {}
    ~basic_socket() {
        if (this->d_id != ::beman::net::detail::socket_id::invalid) {
            ::std::error_code error{};
            this->d_context->release(this->d_id, error);
        }
    }
    auto get_scheduler() noexcept -> scheduler_type { return scheduler_type{this->d_context}; }
    auto id() const -> ::beman::net::detail::socket_id { return this->d_id; }
};

// ----------------------------------------------------------------------------

#endif
