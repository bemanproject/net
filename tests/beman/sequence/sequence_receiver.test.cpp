// tests/beman/sequence/sequence_receiver.test.cpp                    -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/sequence/detail/sequence_receiver.hpp>
#include <beman/execution/execution.hpp>

namespace ex = beman::execution;
namespace sq = beman::sequence;

// ----------------------------------------------------------------------------

namespace {
struct receiver {
    using receiver_concept = ex::receiver_tag;
};

struct sequence_receiver {
    using receiver_concept = sq::sequence_receiver_tag;
};
} // namespace

int main() {
    static_assert(ex::receiver<receiver>);
    static_assert(ex::receiver<sequence_receiver>);
    static_assert(not sq::sequence_receiver<receiver>);
    static_assert(sq::sequence_receiver<sequence_receiver>);
}
