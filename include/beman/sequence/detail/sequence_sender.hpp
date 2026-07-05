// include/beman/sequence/detail/sequence_sender.hpp                  -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef INCLUDED_INCLUDE_BEMAN_SEQUENCE_DETAIL_SEQUENCE_SENDER
#define INCLUDED_INCLUDE_BEMAN_SEQUENCE_DETAIL_SEQUENCE_SENDER

#include <beman/execution/execution.hpp>

// ----------------------------------------------------------------------------

namespace beman::sequence::detail {
struct sequence_sender_tag : ::beman::execution::sender_tag {};
} // namespace beman::sequence::detail

namespace beman::sequence {
using ::beman::sequence::detail::sequence_sender_tag;

template <typename Sndr>
concept sequence_sender =
    ::beman::execution::sender<Sndr> && std::derived_from<typename Sndr::sender_concept, sequence_sender_tag>;
} // namespace beman::sequence

// ----------------------------------------------------------------------------

#endif
