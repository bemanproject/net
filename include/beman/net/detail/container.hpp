// include/beman/net/detail/container.hpp                           -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef INCLUDED_BEMAN_NET_DETAIL_CONTAINER
#define INCLUDED_BEMAN_NET_DETAIL_CONTAINER

#include <beman/net/detail/netfwd.hpp>
#include <cstddef>
#include <variant>
#include <vector>

// ----------------------------------------------------------------------------

namespace beman::net::detail {
template <typename>
class container;
}

// ----------------------------------------------------------------------------

template <typename Record>
class beman::net::detail::container {
  private:
    ::std::vector<::std::variant<::std::size_t, Record>> records;
    ::std::size_t                                        free{};

  public:
    auto insert(Record r) -> ::beman::net::detail::socket_id;
    auto erase(::beman::net::detail::socket_id id) -> void;
    auto operator[](::beman::net::detail::socket_id id) -> Record&;
};

// ----------------------------------------------------------------------------

template <typename Record>
inline auto beman::net::detail::container<Record>::insert(Record r) -> ::beman::net::detail::socket_id {
    if (this->free == this->records.size()) {
        this->records.emplace_back(::std::move(r));
        return ::beman::net::detail::socket_id(this->free++);
    } else {
        ::std::size_t rc(std::exchange(this->free, ::std::get<0>(this->records[this->free])));
        this->records[rc] = ::std::move(r);
        return ::beman::net::detail::socket_id(rc);
    }
}

template <typename Record>
inline auto beman::net::detail::container<Record>::erase(::beman::net::detail::socket_id id) -> void {
    this->records[::std::size_t(id)] = std::exchange(this->free, ::std::size_t(id));
}

template <typename Record>
inline auto beman::net::detail::container<Record>::operator[](::beman::net::detail::socket_id id) -> Record& {
    assert(this->records[::std::size_t(id)].index() == 1u);
    return ::std::get<1>(this->records[::std::size_t(id)]);
}

// ----------------------------------------------------------------------------

#endif