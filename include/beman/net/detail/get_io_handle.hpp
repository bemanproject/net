// include/beman/net/detail/get_io_handle.hpp                         -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef INCLUDED_INCLUDE_BEMAN_NET_DETAIL_GET_IO_HANDLE
#define INCLUDED_INCLUDE_BEMAN_NET_DETAIL_GET_IO_HANDLE

#include <beman/execution/execution.hpp>

// ----------------------------------------------------------------------------

namespace beman::net::detail {
struct get_io_handle_t : beman::execution::forwarding_query_t {
    template <typename Object>
        requires requires(Object obj, const get_io_handle_t& gih) { obj.query(gih); }
    auto operator()(const Object& obj) const noexcept -> decltype(obj.query(*this)) {
        return obj.query(*this);
    }
};
} // namespace beman::net::detail

namespace beman::net {
using get_io_handle_t = beman::net::detail::get_io_handle_t;
inline constexpr get_io_handle_t get_io_handle{};
} // namespace beman::net

// ----------------------------------------------------------------------------

#endif
