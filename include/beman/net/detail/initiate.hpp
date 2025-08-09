// include/beman/net/detail/initiate.hpp                              -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef INCLUDED_INCLUDE_BEMAN_NET_DETAIL_INITIATE
#define INCLUDED_INCLUDE_BEMAN_NET_DETAIL_INITIATE

#include <beman/net/detail/operations.hpp>
#include <beman/net/detail/preconnection.hpp>
#include <beman/net/detail/internet.hpp>
#include <beman/net/detail/into_expected.hpp>
#include <beman/net/detail/task.hpp>
#include <beman/net/detail/into_expected.hpp>

#include <beman/execution/execution.hpp>
#include <beman/execution/task.hpp>
#include <system_error>
#include <utility>

// ----------------------------------------------------------------------------

namespace beman::net::detail {
class initiate_t {
  public:
    auto operator()(const preconnection& pre) const -> beman::net::task<beman::net::ip::tcp::socket> {
        //-dk:TODO use the destination endpoint from the preconnection
        beman::net::ip::tcp::endpoint ep(net::ip::address_v4::loopback(), pre.local().port());
        beman::net::ip::tcp::socket   client(
            (co_await beman::execution::read_env(beman::net::get_io_handle)).get_io_context(), ep);

        auto exp{co_await (beman::net::async_connect(client) | beman::net::detail::into_expected |
                           beman::execution::then([](auto&& e) {
                               std::cout << "connect completed\n";
                               return std::move(e);
                           }))};
        std::cout << "connect completed\n";
        if (!exp) {
            std::cout << "initiate failed: " << exp.error().message() << "\n";
            co_yield beman::execution::with_error(exp.error());
        }
        std::cout << "initiate succeeded\n";
        co_return std::move(client);
    }
};
} // namespace beman::net::detail

namespace beman::net {
using initiate_t = beman::net::detail::initiate_t;
inline constexpr initiate_t initiate{};
} // namespace beman::net

// ----------------------------------------------------------------------------

#endif
