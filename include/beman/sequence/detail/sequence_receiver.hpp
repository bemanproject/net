// include/beman/sequence/detail/sequence_receiver.hpp                -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef INCLUDED_INCLUDE_BEMAN_SEQUENCE_DETAIL_SEQUENCE_RECEIVER
#define INCLUDED_INCLUDE_BEMAN_SEQUENCE_DETAIL_SEQUENCE_RECEIVER

#include <beman/execution/execution.hpp>
#include <type_traits>

// ----------------------------------------------------------------------------

namespace beman::sequence::detail {
struct sequence_receiver_tag : ::beman::execution::receiver_tag {};
} // namespace beman::sequence::detail

namespace beman::sequence {
using ::beman::sequence::detail::sequence_receiver_tag;

template <typename T>
concept sequence_receiver =
    ::beman::execution::receiver<T> &&
    ::std::derived_from<typename std::remove_cvref_t<T>::receiver_concept, ::beman::sequence::sequence_receiver_tag>;
} // namespace beman::sequence

// ----------------------------------------------------------------------------

#endif
