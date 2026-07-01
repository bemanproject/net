// tests/beman/sequence/iota.pass.cpp                                 -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/sequence/detail/iota.hpp>
#include <beman/execution/execution.hpp>

namespace ex = beman::execution;
namespace sq = beman::sequence;

// ----------------------------------------------------------------------------

int main() {
    auto sndr = sq::iota(1, 7);
    static_assert(ex::sender<decltype(sndr)>);
    static_assert(ex::sender_in<decltype(sndr)>);
    static_assert(sq::sequence_sender<decltype(sndr)>);

    ex::sync_wait(std::move(sndr));
    ex::sync_wait(sndr);
}
