// tests/beman/sequence/filter_each.test.cpp                          -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/sequence/detail/filter_each.hpp>
#include <beman/sequence/detail/then_each.hpp>
#include <beman/sequence/detail/iota.hpp>
#include <beman/sequence/detail/ignore_all.hpp>
#include <cassert>
#include <vector>

namespace ex = beman::execution;
namespace sq = beman::sequence;

// ----------------------------------------------------------------------------

int main() {
    //-dk:TODO test the actual filter_each functionality
    std::vector<int> v;
    assert(v.empty());
    ex::sync_wait(sq::ignore_all(sq::then_each(sq::filter_each(sq::iota(0, 10), [](int i) noexcept { return i % 2 == 0; }), [&v](int i) noexcept { v.push_back(i); })));
    assert(v.size() == 10u);
    for (int i = 0; i < 10; ++i) {
        assert(v[i] == i);
    }
}
