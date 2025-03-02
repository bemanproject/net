// examples/http-server.cpp                                           -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/net/net.hpp>
#include <beman/execution/execution.hpp>
#include "demo_algorithm.hpp"
#include "demo_scope.hpp"
#include "demo_task.hpp"
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <string_view>
#include <unordered_map>

namespace ex  = beman::execution;
namespace net = beman::net;
using namespace std::chrono_literals;

// ----------------------------------------------------------------------------

std::unordered_map<std::string, std::string> files{
    {"/", "examples/data/index.html"},
    {"/favicon.ico", "examples/data/favicon.ico"},
    {"/logo.png", "examples/data/logo.png"},
};

auto process_request(auto& stream, std::string request) -> demo::task<> {
    std::istringstream in(request);
    std::string        method, url, version;
    if (!(in >> method >> url >> version) || method != "GET") {
        std::cout << "not a [supported] HTTP request\n";
        co_return;
    }
    auto it(files.find(url));
    std::cout << "url='" << url << "' -> " << (it == files.end() ? "not found" : it->second) << "\n";
    std::ostringstream out;
    out << std::ifstream(it == files.end() ? std::string() : it->second).rdbuf();
    auto body{out.str()};
    out.clear();
    out.str(std::string());
    out << "HTTP/1.1 " << (it == files.end() ? "404 not found" : "200 found\r\n") << "Content-Length: " << body.size()
        << "\r\n\r\n"
        << body;
    auto response(out.str());
    co_await net::async_send(stream, net::buffer(response));
}

auto timeout(auto scheduler, auto duration, auto sender) {
    return demo::when_any(std::move(sender),
                          net::resume_after(scheduler, duration) | demo::into_error([] { return std::error_code(); }));
}

auto make_client(auto scheduler, auto stream) -> demo::task<> {
    char        buffer[16];
    std::string request;
    try {
        while (auto n = co_await timeout(scheduler, 3s, net::async_receive(stream, net::buffer(buffer)))) {
            std::string_view sv(buffer, n);
            request += sv;
            if (request.npos != sv.find("\r\n\r\n")) {
                co_await process_request(stream, std::move(request));
                break;
            }
        }
    } catch (...) {
        std::cout << "ex: timeout\n";
    }
    std::cout << "client done\n";
}

auto main() -> int {
    net::io_context        context;
    demo::scope            scope;
    net::ip::tcp::endpoint ep(net::ip::address_v4::any(), 12345);
    net::ip::tcp::acceptor server(context, ep);
    std::cout << "listening on " << ep << "\n";

    scope.spawn(std::invoke(
        [](auto scheduler, auto& scp, auto& svr) -> demo::task<> {
            while (true) {
                auto [stream, address] = co_await net::async_accept(svr);
                std::cout << "received connection from " << address << "\n";
                scp.spawn(make_client(scheduler, std::move(stream)));
            }
        },
        context.get_scheduler(),
        scope,
        server));

    context.run();
}
