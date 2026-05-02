// examples/demo_http.hpp                                             -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef INCLUDED_EXAMPLES_DEMO_HTTP
#define INCLUDED_EXAMPLES_DEMO_HTTP

#include <beman/execution/execution.hpp>
#include <beman/execution/task.hpp>
#include <beman/net/net.hpp>
#include "demo_stream.hpp"
#include <algorithm>
#include <array>
#include <cstddef>
#include <string_view>

// ----------------------------------------------------------------------------

namespace demo::http {
    namespace ex  = ::beman::execution;
    namespace net = ::beman::net;

    template <typename Stream>
    struct http_client {
        Stream stream;
    public:
        http_client(Stream s): stream{std::move(s)} {}
        auto request() ->ex::task<> {
            std::vector<char> method, url, version;
            if (!(co_await stream.read(method, ' '))) {
                co_return;
            }
            std::cout << "read method='" << std::string_view(method) << "'\n";

            if (!(co_await stream.read(url, ' '))) {
                co_return;
            }
            std::cout << "read url='" << std::string_view(url) << "'\n";

            if (!(co_await stream.read(version, "\r\n"))) {
                co_return;
            }
            std::cout << "read version='" << std::string_view(version) << "'\n";

            std::vector<char> header;
            while (co_await stream.read(header, "\r\n") && !header.empty()) {
                std::cout << "read header line='" << std::string_view(header) << "'\n";
                header.clear();
            }
            std::cout << "read HTTP GET request\n";

            co_return;
        }
    };

    struct no_error_env {
        using error_types = ::beman::execution::completion_signatures<>;
    };

    template <typename SenderFactory>
    ::beman::execution::task<void, demo::http::no_error_env>
    http_server(::beman::net::io_context& io, unsigned short port, SenderFactory fun) {
        ::beman::net::ip::tcp::endpoint ep(::beman::net::ip::address_v4::any(), port);
        ::beman::net::ip::tcp::acceptor server(io, ep);
        while (true) {
            auto[client, addr] = co_await ::beman::net::async_accept(server);
            std::cout << "connection from " << addr << "\n";
            fun(demo::http::http_client(demo::tcp_stream(std::move(client))));
        }
    }
}

namespace demo {
    using http::http_client;
    using http::http_server;
}

// ----------------------------------------------------------------------------

#endif
