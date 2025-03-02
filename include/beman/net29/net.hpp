// include/beman/net29/net.hpp                                        -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef INCLUDED_INCLUDE_BEMAN_NET29_NET
#define INCLUDED_INCLUDE_BEMAN_NET29_NET

#include <beman/net/net.hpp>
namespace beman::net29::detail {
namespace ex = beman::net::detail::ex;
}
namespace beman::net29 {
namespace ip = ::beman::net::ip;

using ::beman::net::async_accept;
using ::beman::net::async_connect;
using ::beman::net::async_receive;
using ::beman::net::async_send;
using ::beman::net::buffer;
using ::beman::net::const_buffer;
using ::beman::net::io_context;
using ::beman::net::resume_after;
using ::beman::net::resume_at;

} // namespace beman::net29

#endif
