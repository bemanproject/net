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

int main(int, char*[]) {
    std::cout << std::unitbuf;
    net::scope scope;

    std::cout << "spawning\n";
    ex::spawn(
        []() -> net::task<> {
            net::preconnection pre(net::remote_endpoint().with_hostname("example.com").with_port(80));
            std::cout << "initiate\n";
            auto        socket{co_await net::initiate(pre)};
            std::string request = "GET / HTTP/1.1\r\nHost: example.com\r\nConnection: close\r\n\r\n";
            std::cout << "async_send\n";
            co_await net::async_send(socket, net::buffer(request));

            char buffer[1024];
            std::cout << "reading\n";
            for (std::size_t n; (n = co_await net::async_receive(socket, net::buffer(buffer)));) {
                std::cout << "read n=" << n << "\n";
                std::cout.write(buffer, n);
            }
        }(),
        scope.get_token());

    std::cout << "spawned\n";

    ex::sync_wait(scope.run());
}