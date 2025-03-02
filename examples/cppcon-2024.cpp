// examples/http-server.cpp                                           -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/net/net.hpp>
#include <beman/execution/execution.hpp>
#include "demo_algorithm.hpp"
#include "demo_error.hpp"
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

auto process(auto& stream, const auto& request) -> demo::task<> {
    std::cout << "request=" << request << "\n";
    std::string        method, url, version;
    std::string        body;
    std::ostringstream out;
    if (std::istringstream(request) >> method >> url >> version && files.contains(url)) {
        std::cout << "url=" << url << "\n";
        out << std::ifstream(files[url]).rdbuf();
        body = out.str();
        out.str({});
    }

    out << "HTTP/1.1 " << (body.empty() ? "404 not found" : "200 found") << "\r\n"
        << "Content-Length: " << body.size() << "\r\n"
        << "\r\n"
        << body;
    auto response{out.str()};
    co_await net::async_send(stream, net::buffer(response));
}

auto timeout(auto scheduler, auto duration, auto sender) {
    return demo::when_any(std::move(sender), net::resume_after(scheduler, duration) | demo::into_error([] {
                                                 return std::error_code(demo::timeout, demo::category());
                                             }));
}

auto make_client_handler(auto scheduler, auto stream) -> demo::task<> {
    char        buffer[16];
    std::string request;
    while (true)
        try {
            if (auto n = co_await timeout(scheduler, 2s, net::async_receive(stream, net::buffer(buffer)))) {
                std::string_view data(buffer, n);
                request += data;
                if (request.find("\r\n\r\n") != request.npos) {
                    co_await process(stream, request);
                    break;
                }
            } else {
                // std::cout << "ERROR (via expected): " << std::get<0>(n.error()).message() << "\n";
                break;
            }
        } catch (const std::variant<std::error_code>& ex) {
            std::cout << "ERROR: " << std::get<0>(ex).message() << "\n";
            break;
        }

    co_return;
}

auto main() -> int {
    demo::scope scope;

    net::io_context        context;
    net::ip::tcp::endpoint endpoint(net::ip::address_v4::any(), 12345);
    net::ip::tcp::acceptor acceptor(context, endpoint);

    scope.spawn(std::invoke(
        [](auto scheduler, auto& scp, auto& acc) -> demo::task<> {
            while (true) {
                auto [stream, address] = co_await net::async_accept(acc);
                std::cout << "received client: " << address << "\n";
                scp.spawn(make_client_handler(scheduler, std::move(stream)));
            }
        },
        context.get_scheduler(),
        scope,
        acceptor));

    context.run();
}
