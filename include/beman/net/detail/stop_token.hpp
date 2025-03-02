// include/beman/net/detail/stop_token.hpp                          -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef INCLUDED_BEMAN_NET_DETAIL_STOP_TOKEN
#define INCLUDED_BEMAN_NET_DETAIL_STOP_TOKEN

#include <beman/execution/stop_token.hpp>

// ----------------------------------------------------------------------------

namespace beman::net::detail::ex {
using ::beman::execution::inplace_stop_source;
using ::beman::execution::inplace_stop_token;
using ::beman::execution::never_stop_token;
using ::beman::execution::stop_callback_for_t;
using ::beman::execution::detail::stoppable_callback_for;
} // namespace beman::net::detail::ex

// ----------------------------------------------------------------------------

#endif
