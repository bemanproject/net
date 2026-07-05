// tests/beman/sequence/sequence_sender.pass.cpp                      -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/sequence/detail/sequence_sender.hpp>
#include <beman/execution/execution.hpp>
#include <cassert>
#include <concepts>

namespace ex = beman::execution;
namespace sq = beman::sequence;

// ----------------------------------------------------------------------------

namespace {
struct sender {
    using sender_concept = ex::sender_tag;
};
static_assert(ex::sender<sender>);
static_assert(not sq::sequence_sender<sender>);

struct sequence_sender {
    using sender_concept = sq::sequence_sender_tag;
};
static_assert(ex::sender<sequence_sender>);
static_assert(sq::sequence_sender<sequence_sender>);
} // namespace

int main() { static_assert(std::derived_from<sq::sequence_sender_tag, ex::sender_tag>); }
