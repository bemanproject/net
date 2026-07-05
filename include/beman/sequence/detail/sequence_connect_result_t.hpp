// include/beman/sequence/detail/sequence_connect_result_t.hpp        -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef INCLUDED_INCLUDE_BEMAN_SEQUENCE_DETAIL_SEQUENCE_CONNECT_RESULT
#define INCLUDED_INCLUDE_BEMAN_SEQUENCE_DETAIL_SEQUENCE_CONNECT_RESULT

#include <beman/sequence/detail/sequence_connect.hpp>
#include <beman/sequence/detail/sequence_receiver.hpp>
#include <beman/sequence/detail/sequence_sender.hpp>
#include <type_traits>

// ----------------------------------------------------------------------------

namespace beman::sequence {
template <::beman::sequence::sequence_sender Sndr, ::beman::sequence::sequence_receiver Rcvr>
using sequence_connect_result_t =
    decltype(::beman::sequence::sequence_connect(::std::declval<Sndr>(), ::std::declval<Rcvr>()));
}

// ----------------------------------------------------------------------------

#endif
