// include/beman/net/detail/rendezvous.hpp                            -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef INCLUDED_INCLUDE_BEMAN_NET_DETAIL_RENDEZVOUS
#define INCLUDED_INCLUDE_BEMAN_NET_DETAIL_RENDEZVOUS

// ----------------------------------------------------------------------------

namespace beman::net::detail {
struct rendezvous_t {};
} // namespace beman::net::detail

namespace beman::net {
using rendezvous_t = beman::net::detail::rendezvous_t;
inline constexpr rendezvous_t rendezvous{};
} // namespace beman::net

// ----------------------------------------------------------------------------

#endif
