// include/beman/sequence/detail/connector.hpp                        -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef INCLUDED_INCLUDE_BEMAN_SEQUENCE_DETAIL_CONNECTOR
#define INCLUDED_INCLUDE_BEMAN_SEQUENCE_DETAIL_CONNECTOR

#include <beman/execution/execution.hpp>
#include <utility>

// ----------------------------------------------------------------------------

namespace beman::sequence::detail {
template <::beman::execution::sender Sndr, ::beman::execution::receiver Rcvr>
struct connector {
    using state_t = ::beman::execution::connect_result_t<Sndr, Rcvr>;
    state_t st;
    connector(Sndr&& sndr, Rcvr&& rcvr) noexcept
        : st(::beman::execution::connect(::std::forward<Sndr>(sndr), ::std::forward<Rcvr>(rcvr))) {}

    void start() & noexcept { ::beman::execution::start(this->st); }
};
} // namespace beman::sequence::detail

// ----------------------------------------------------------------------------

#endif
