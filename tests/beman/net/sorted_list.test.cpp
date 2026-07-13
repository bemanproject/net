// src/beman/net/tests/sorted_list.pass.cpp                           -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/net/detail/sorted_list.hpp>
#include <cassert>
#include <iostream>

// ----------------------------------------------------------------------------

namespace {
auto use(auto&&...) {}
struct node {
    int   value{};
    node* next{};
};

auto print(node* n) {
    std::cout << "[";
    for (; n; n = n->next)
        std::cout << n->value << " ";
    std::cout << "]\n";
}

auto test_insert() {
    std::cout << "test insert()\n";
    node                                  d{};
    node                                  n[] = {{3, &d}, {2, &d}, {5, &d}, {4, &d}};
    beman::net::detail::sorted_list<node> l;
    assert(l.empty());
    print(l.front());

    assert(n[0].next != nullptr);
    l.insert(n + 0);
    print(l.front());
    assert(l.front() == n + 0);
    assert(n[0].next == nullptr);

    l.insert(n + 1);
    print(l.front());
    assert(l.front() == n + 1);
    assert(n[1].next == n + 0);

    l.insert(n + 2);
    print(l.front());
    assert(l.front() == n + 1);
    assert(n[0].next == n + 2);
    assert(n[2].next == nullptr);

    l.insert(n + 3);
    print(l.front());
    assert(l.front() == n + 1);
    assert(n[3].next == n + 2);
    assert(n[0].next == n + 3);
    assert(n[2].next == nullptr);
}

auto test_pop_front() -> void {
    node                                  n[] = {{3}, {2}, {5}, {4}};
    beman::net::detail::sorted_list<node> l;
    for (node& c : n)
        l.insert(&c);

    std::cout << "test pop_front()\n";
    print(l.front());
    assert(n + 1 == l.front());
    assert(n + 1 == l.pop_front());

    print(l.front());
    assert(n + 0 == l.front());
    assert(n + 0 == l.pop_front());

    print(l.front());
    assert(n + 3 == l.front());
    assert(n + 3 == l.pop_front());

    print(l.front());
    assert(n + 2 == l.front());
    assert(n + 2 == l.pop_front());

    print(l.front());
    assert(l.empty());
}

auto test_pop_front_or() -> void {
    node                                  n[] = {{3}, {2}, {5}, {4}};
    beman::net::detail::sorted_list<node> l;
    for (node& c : n)
        l.insert(&c);

    assert(l.pop_front_or(-1) == 2);
    assert(l.pop_front_or(-1) == 3);
    assert(l.pop_front_or(-1) == 4);
    assert(l.pop_front_or(-1) == 5);
    assert(l.pop_front_or(-1) == -1);
}

auto test_erase() -> void {
    std::cout << "test erase()\n";
    node                                  n[] = {{2}, {1}, {5}, {4}, {3}};
    beman::net::detail::sorted_list<node> l;
    for (node& c : n)
        l.insert(&c);
    print(l.front());

    assert(n + 1 == l.front());
    assert(l.erase(n + 1) == n + 1);
    print(l.front());
    assert(n + 0 == l.front());

    assert(n[3].next == n + 2);
    assert(l.erase(n + 2) == n + 2);
    print(l.front());
    assert(n + 0 == l.front());
    assert(n[3].next == nullptr);

    assert(n[0].next == n + 4);
    assert(l.erase(n + 4) == n + 4);
    print(l.front());
    assert(n + 0 == l.front());
    assert(n[0].next == n + 3);

    assert(l.erase(n + 4) == nullptr);
}
} // namespace

// ----------------------------------------------------------------------------

auto main() -> int {
    test_insert();
    test_pop_front();
    test_pop_front_or();
    test_erase();
}
