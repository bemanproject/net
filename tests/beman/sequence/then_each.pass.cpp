// tests/beman/sequence/then_each.pass.cpp                            -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/sequence/detail/then_each.hpp>
#include <beman/sequence/detail/iota.hpp>
#include <beman/sequence/detail/ignore_all.hpp>
#include <cassert>
#include <iostream> //-dk:TODO remove
#include <vector>

namespace ex = beman::execution;
namespace sq = beman::sequence;

// ----------------------------------------------------------------------------

int main() {
    std::vector<int> v;
    assert(v.empty());
    ex::sync_wait(sq::ignore_all(sq::then_each(sq::iota(0, 10), [&v](int i) noexcept { v.push_back(i); })));
    assert(v.size() == 10u);
    for (int i = 0; i < 10; ++i) {
        assert(v[i] == i);
    }
}
