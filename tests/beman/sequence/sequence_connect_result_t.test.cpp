// tests/beman/sequence/sequence_connect_result_t.test.cpp            -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/sequence/detail/sequence_connect_result_t.hpp>
#include <beman/execution/execution.hpp>
#include <concepts>

namespace ex = ::beman::execution;
namespace sq = ::beman::sequence;

// ----------------------------------------------------------------------------

namespace {
struct test_state {
    using operation_state_concept = ex::operation_state_tag;
    void start() & noexcept {}
};

struct sequence_sender {
    using sender_concept = sq::sequence_sender_tag;

    auto sequence_connect(sq::sequence_receiver auto&&) { return test_state{}; }
};

struct sequence_receiver {
    using receiver_concept = sq::sequence_receiver_tag;
};

} // namespace

int main() {
    static_assert(ex::operation_state<test_state>);
    static_assert(sq::sequence_sender<sequence_sender>);
    static_assert(sq::sequence_receiver<sequence_receiver>);
    static_assert(::std::same_as<sq::sequence_connect_result_t<sequence_sender, sequence_receiver>, test_state>);
}
