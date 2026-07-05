// include/beman/sequence/detail/set_next.hpp                         -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef INCLUDED_INCLUDE_BEMAN_SEQUENCE_DETAIL_SET_NEXT
#define INCLUDED_INCLUDE_BEMAN_SEQUENCE_DETAIL_SET_NEXT

#include <beman/execution/execution.hpp>

// ----------------------------------------------------------------------------

namespace beman::sequence::detail {
struct set_next_t {
    template <::beman::execution::receiver Rcvr, ::beman::execution::sender Sndr>
    auto operator()(Rcvr& rcvr, Sndr&& sndr) const noexcept {
        return rcvr.set_next(std::forward<Sndr>(sndr));
    }
};
} // namespace beman::sequence::detail

namespace beman::sequence {
using beman::sequence::detail::set_next_t;
inline constexpr beman::sequence::set_next_t set_next{};
} // namespace beman::sequence

// ----------------------------------------------------------------------------

#endif
