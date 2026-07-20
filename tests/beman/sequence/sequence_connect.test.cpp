// tests/beman/sequence/sequence_connect.test.cpp                     -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/sequence/detail/sequence_connect.hpp>
#include <beman/execution/execution.hpp>

namespace ex = beman::execution;
namespace sq = beman::sequence;

// ----------------------------------------------------------------------------

namespace {
struct state {
    using operation_state_concept = ex::operation_state_tag;
    void start() & noexcept {}
};

struct sender {
    using sender_concept = ex::sender_tag;
};

struct sequence_sender {
    using sender_concept = sq::sequence_sender_tag;

    auto sequence_connect(sq::sequence_receiver auto&&) { return state{}; }
};

struct receiver {
    using receiver_concept = ex::receiver_tag;
};

struct sequence_receiver {
    using receiver_concept = sq::sequence_receiver_tag;
};

template <typename Sndr, typename Rcvr>
void test_sequence_connect() {
    constexpr bool expect = sq::sequence_sender<Sndr> && sq::sequence_receiver<Rcvr>;
    static_assert(expect == requires { sq::sequence_connect(Sndr{}, Rcvr{}); });

    if constexpr (expect) {
        auto st{sq::sequence_connect(Sndr{}, Rcvr{})};
        static_assert(ex::operation_state<decltype(st)>);
    }
}
} // namespace

int main() {
    static_assert(ex::operation_state<state>);

    static_assert(ex::sender<sender>);
    static_assert(ex::sender<sequence_sender>);
    static_assert(not sq::sequence_sender<sender>);
    static_assert(sq::sequence_sender<sequence_sender>);

    static_assert(ex::receiver<receiver>);
    static_assert(ex::receiver<sequence_receiver>);
    static_assert(not sq::sequence_receiver<receiver>);
    static_assert(sq::sequence_receiver<sequence_receiver>);

    test_sequence_connect<sender, receiver>();
    test_sequence_connect<sender, sequence_receiver>();
    test_sequence_connect<sequence_sender, receiver>();
    test_sequence_connect<sequence_sender, sequence_receiver>();
}
