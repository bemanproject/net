// include/beman/net/detail/netfwd.hpp                              -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef INCLUDED_BEMAN_NET_DETAIL_NETFWD
#define INCLUDED_BEMAN_NET_DETAIL_NETFWD

#include <limits>
#include <cstdint>

// ----------------------------------------------------------------------------

namespace beman::net::detail {
enum socket_id : ::std::uint_least32_t { invalid = ::std::numeric_limits<::std::uint_least32_t>::max() };
struct context_base;
using native_handle_type = int;
inline constexpr native_handle_type invalid_handle{-1};
} // namespace beman::net::detail

namespace beman::net {
class io_context;
class socket_base;
template <typename>
class basic_socket;
template <typename>
class basic_stream_socket;
template <typename>
class basic_socket_acceptor;
namespace ip {
template <typename>
class basic_endpoint;
class tcp;
class address;
class address_v4;
class address_v6;
} // namespace ip
} // namespace beman::net

// ----------------------------------------------------------------------------

#endif
