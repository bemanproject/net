// include/beman/net/detail/get_scope_token.hpp                       -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef INCLUDED_INCLUDE_BEMAN_NET_DETAIL_GET_SCOPE_TOKEN
#define INCLUDED_INCLUDE_BEMAN_NET_DETAIL_GET_SCOPE_TOKEN

// ----------------------------------------------------------------------------

namespace beman::net::detail {
    struct get_scope_token_t {
        template <typename Object>
        auto operator()(Object const& obj) const -> decltype(obj.query(*this)) {
            return obj.query(*this);
        }
    };
}
namespace beman::net {
using get_scope_token_t = beman::net::detail::get_scope_token_t;
inline constexpr get_scope_token_t get_scope_token{};
}

// ----------------------------------------------------------------------------

#endif
