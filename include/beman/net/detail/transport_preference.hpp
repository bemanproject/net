// include/beman/net/detail/transport_preference.hpp                  -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef INCLUDED_INCLUDE_BEMAN_NET_DETAIL_TRANSPORT_PREFERENCE
#define INCLUDED_INCLUDE_BEMAN_NET_DETAIL_TRANSPORT_PREFERENCE

// ----------------------------------------------------------------------------

namespace beman::net::detail {
enum class transport_preference { require, prefer, none, avoid, prohibit };
}

// ----------------------------------------------------------------------------

#endif
