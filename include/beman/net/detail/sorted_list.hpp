// include/beman/net/detail/sorted_list.hpp                           -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef INCLUDED_BEMAN_NET_DETAIL_SORTED_LIST
#define INCLUDED_BEMAN_NET_DETAIL_SORTED_LIST

#include <functional>
#include <type_traits>

// ----------------------------------------------------------------------------

namespace beman::net::detail {
template <typename T, typename = ::std::less<>, typename = decltype([](T* node) { return node->value; })>
struct sorted_list;
} // namespace beman::net::detail

// ----------------------------------------------------------------------------

template <typename T, typename Compare, typename Value>
struct beman::net::detail::sorted_list {
    using next_t = decltype(::std::declval<T>().next);

    static constexpr auto value{[v = Value()](next_t n) { return v(static_cast<T*>(n)); }};
    Compare               comp{};
    next_t                head{};

    auto empty() const -> bool { return this->head == nullptr; }
    auto front() const -> T* { return static_cast<T*>(this->head); }
    auto value_or(auto&& v) const { return this->empty() ? v : this->value(this->head); }
    auto pop_front() -> T*;
    auto pop_front_or(auto&& v) { return this->empty() ? v : this->value(this->pop_front()); }
    auto insert(T* node) -> T*;
    auto erase(next_t) -> next_t;
};

template <typename T, typename Compare, typename Value>
auto beman::net::detail::sorted_list<T, Compare, Value>::pop_front() -> T* {
    next_t rc{this->head};
    this->head = rc->next;
    return static_cast<T*>(rc);
}

template <typename T, typename Compare, typename Value>
auto beman::net::detail::sorted_list<T, Compare, Value>::insert(T* node) -> T* {
    next_t* it{&this->head};
    while (*it != nullptr && this->comp(this->value(*it), this->value(node)))
        it = &(*it)->next;
    node->next = *it;
    *it        = node;
    return node;
}

template <typename T, typename Compare, typename Value>
auto beman::net::detail::sorted_list<T, Compare, Value>::erase(next_t node) -> next_t {
    next_t* it{&this->head};
    while (*it != node && *it != nullptr)
        it = &(*it)->next;
    if (*it == node) {
        *it = node->next;
        return node;
    }
    return nullptr;
}

// ----------------------------------------------------------------------------

#endif
