// include/beman/sequence/detail/sequence_connect.hpp                 -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef INCLUDED_INCLUDE_BEMAN_SEQUENCE_DETAIL_SEQUENCE_CONNECT
#define INCLUDED_INCLUDE_BEMAN_SEQUENCE_DETAIL_SEQUENCE_CONNECT

#include <beman/sequence/detail/sequence_receiver.hpp>
#include <beman/sequence/detail/sequence_sender.hpp>

#include <utility>

// ----------------------------------------------------------------------------

namespace beman::sequence::detail {
struct sequence_connect_t {
    template <::beman::sequence::sequence_sender Sndr, ::beman::sequence::sequence_receiver Rcvr>
    auto operator()(Sndr&& sndr, Rcvr&& rcvr) const noexcept {
        return ::std::forward<Sndr>(sndr).sequence_connect(::std::forward<Rcvr>(rcvr));
    }
};
} // namespace beman::sequence::detail

namespace beman::sequence {
using ::beman::sequence::detail::sequence_connect_t;
inline constexpr sequence_connect_t sequence_connect{};
} // namespace beman::sequence

// ----------------------------------------------------------------------------

#endif
