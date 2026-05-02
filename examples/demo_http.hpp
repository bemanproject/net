// examples/demo_http.hpp                                             -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef INCLUDED_EXAMPLES_DEMO_HTTP
#define INCLUDED_EXAMPLES_DEMO_HTTP

#include <beman/execution/execution.hpp>
#include <beman/execution/task.hpp>
#include <beman/net/net.hpp>
#include <algorithm>
#include <array>
#include <cstddef>
#include <string_view>

// ----------------------------------------------------------------------------

namespace demo::http {
    namespace ex  = ::beman::execution;
    namespace net = ::beman::net;
    struct async_stream {
        using buffer_t = ::std::array<char, 1024>;
        using buffer_iterator = typename buffer_t::iterator;

        ::beman::net::ip::tcp::socket socket;
        buffer_t      buffer{};
        buffer_iterator it{buffer.begin()};
        buffer_iterator end{buffer.end()};

        static constexpr std::size_t length(char) { return 1u; }
        bool consume(auto& to, char sentinel) {
            auto end{::std::find(this->it, this->end, sentinel)};
            to.insert(to.end(), this->it, end);
            this->it = end;
            return this->it == this->end;
        }
        template <::std::size_t Size>
        static constexpr std::size_t length(const char (&)[Size]) { return Size - 1u; }
        template <::std::size_t Size>
        bool consume(auto& to, const char (&sentinel)[Size]) {
            auto end{::std::search(this->it, this->end, sentinel, sentinel + Size - 1)};
            if (end == this->end) {
                end -= std::min(std::size_t(end - this->it), Size);
                to.insert(to.end(), this->it, end);
                std::cout << "insert(" << std::string_view(this->it, end) << ") - continue\n";
                this->it = this->buffer.begin();
                this->end = std::move(end, this->end, this->it);
                return true;
            }
            else {
                to.insert(to.end(), this->it, end);
                std::cout << "insert(" << std::string_view(this->it, end) << ") - done\n";
                this->it = end;
                return false;
            }
        }
        auto read(auto& to, const auto& sentinel)->ex::task<bool> {
            while (this->consume(to, sentinel)) {
                this->it = this->buffer.begin();
                std::size_t n{co_await net::async_receive(this->socket, net::buffer(this->buffer))};
                if (n == 0) {
                    co_return false;
                }
                std::cout << "received='" << std::string_view(this->it, n) << "'\n";
                this->end = this->it + n;
            }
            this->it += length(sentinel);

            co_return true;
        }
    };

    struct http_client {
        ::demo::http::async_stream stream;
    public:
        http_client(::beman::net::ip::tcp::socket s, auto): stream{std::move(s)} {}
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
            fun(demo::http::http_client(std::move(client), std::move(addr)));
        }
    }
}

namespace demo {
    using http::http_client;
    using http::http_server;
}

// ----------------------------------------------------------------------------

#endif
