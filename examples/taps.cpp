// examples/taps.cpp                                                  -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/execution/execution.hpp>
#include <beman/net/net.hpp>
#include <iostream>
#include <system_error>
#include <string>

namespace ex  = beman::execution;
namespace net = beman::net;

// ----------------------------------------------------------------------------

int main(int ac, char* av[]) {
    net::scope scope;

    net::preconnection pre(
        net::remote_endpoint().with_hostname(1 < ac ? av[1] : "example.com").with_port(2 < ac ? std::stoi(av[2]) : 80),
        {});

    ex::spawn(net::initiate(pre) | ex::then([](auto) noexcept {}) | ex::upon_error([](auto) noexcept {}),
              scope.get_token());

    ex::spawn(
        []() -> net::task<void> {
            [[maybe_unused]] auto handle = co_await ex::read_env(net::get_io_handle);

            auto        socket{co_await net::initiate(
                net::preconnection(net::remote_endpoint().with_hostname("example.com").with_port(80)))};
            std::string request = "GET / HTTP/1.1\r\nHost: example.com\r\nConnection: close\r\n\r\n";
            co_await net::async_send(socket, net::buffer(request));

            char buffer[1024];
            for (std::size_t n; (n = co_await net::async_receive(socket, net::buffer(buffer)));) {
                std::cout.write(buffer, n);
            }
        }(),
        scope.get_token());

    ex::sync_wait(scope.run());
}