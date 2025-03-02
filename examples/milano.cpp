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
    {"/", "examples/data/index-milano.html"},
    {"/favicon.ico", "examples/data/favicon.ico"},
    {"/logo.png", "examples/data/logo.png"},
    {"/itcpp.png", "examples/data/itcpp.png"},
};

auto timeout(auto scheduler, auto dur, ex::sender auto sender) {
    return demo::when_any(net::resume_after(scheduler, dur) | demo::into_error([] { return std::error_code(); }),
                          std::move(sender));
}

using on_exit_msg = std::unique_ptr<const char, decltype([](auto msg) { std::cout << msg << "\n"; })>;
demo::task<> run_client(auto client, auto s) {
    on_exit_msg        exit("client exiting");
    char               buffer[8194];
    std::ostringstream out;
    try {
        while (true) {
            auto n = co_await timeout(s, 1s, net::async_receive(client, net::buffer(buffer)));
            if (n == 0u)
                co_return;
            // std::cout << "received=" << std::string_view(buffer, n) << "\n";
            std::istringstream in(std::string(buffer, n));
            std::string        method, url, version;
            if (!(in >> method >> url >> version))
                co_return;
            auto it = files.find(url);
            std::cout << "url=" << url << " found=" << (it == files.end() ? "no" : "yes") << "\n";
            std::string content;
            if (it != files.end()) {
                std::ifstream fin(it->second);
                out.str({});
                out << fin.rdbuf();
                content = out.str();
            }
            out.str(std::string());
            out << "HTTP/1.1 200 found\r\n"
                << "Content-Length: " << content.size() << "\r\n"
                << "\r\n"
                << content;

            content = out.str();
            co_await net::async_send(client, net::buffer(content));
            // break;
        }
    } catch (const std::exception& ex) {
        std::cout << "received timeout! ex=" << ex.what() << "\n";
    } catch (...) {
        std::cout << "received timeout!\n";
    }
}

auto main() -> int {
    demo::scope     scope;
    net::io_context context;

    scope.spawn([](auto& ctxt, auto& scp) -> demo::task<> {
        net::ip::tcp::endpoint ep(net::ip::address_v4::any(), 12345);
        net::ip::tcp::acceptor acceptor(ctxt, ep);
        while (true) {
            auto [client, address] = co_await net::async_accept(acceptor);
            std::cout << "received a connection from " << address << "\n";
            scp.spawn(run_client(std::move(client), ctxt.get_scheduler()));
        }
    }(context, scope));

    context.run();
}
