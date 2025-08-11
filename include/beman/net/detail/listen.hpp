// include/beman/net/detail/listen.hpp                                -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef INCLUDED_INCLUDE_BEMAN_NET_DETAIL_LISTEN
#define INCLUDED_INCLUDE_BEMAN_NET_DETAIL_LISTEN

// ----------------------------------------------------------------------------

namespace beman::net::detail {
struct listen_t {};
} // namespace beman::net::detail

namespace beman::net {
using listen_t = beman::net::detail::listen_t;
inline constexpr listen_t listen{};
} // namespace beman::net

// ----------------------------------------------------------------------------

#endif
