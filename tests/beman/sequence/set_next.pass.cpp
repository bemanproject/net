// tests/beman/sequence/set_next.pass.cpp                             -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/sequence/detail/set_next.hpp>
#include <beman/execution/execution.hpp>
#include <cassert>
#include <utility>

namespace ex = beman::execution;
namespace sq = beman::sequence;

// ----------------------------------------------------------------------------

namespace {
    struct receiver {
        using receiver_concept = ex::receiver_tag;
        bool& done;
        auto set_next(ex::sender auto sndr) noexcept {
            this->done = true;
            return std::move(sndr);
        }
    };
    static_assert(ex::receiver<receiver>);
}

int main() {
    bool done = false;
    receiver r{done};
    assert(not done);
    [[maybe_unused]] ex::sender auto sndr = sq::set_next(r, ex::just());
    assert(done);
}
