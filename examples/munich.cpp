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
    {"/", "examples/data/index-munich.html"},
    {"/favicon.ico", "examples/data/favicon.ico"},
    {"/logo.png", "examples/data/logo.png"},
    {"/muc.png", "examples/data/muc.png"},
};

auto timeout(ex::scheduler auto scheduler, auto dur, ex::sender auto sender) {
    return demo::when_any(
        std::move(sender),
        net::resume_after(scheduler, dur)
            | ex::then([]{ return std::size_t(); })
            //| ex::let_value([]{ return ex::just_stopped(); })
    );
}

demo::task<> run_client(auto client, auto scheduler)
{
    std::cout << "new client\n";
    char buffer[10000];
    while (std::size_t n = co_await timeout(scheduler, 3s, net::async_receive(client, net::buffer(buffer)))) {
        std::string request(buffer, n);
        std::istringstream in(request);
        std::string method, url, version;
        if ((in >> method >> url >> version) && method == "GET"
             && files.contains(url)) {
            std::cout << "requesting '" << url << "'\n";
            std::ifstream fin(files[url]);
            std::ostringstream out;
            out << fin.rdbuf();
            auto data = out.str();

            out.str(std::string());
            out << "HTTP/1.1 200\r\n"
                << "Content-Length: " << data.size() << "\r\n"
                << "\r\n" << data;
            auto response = out.str();
            co_await net::async_send(client, net::buffer(response));
        }
    }
    std::cout << "client done\n";
}

auto main() -> int {
    std::cout << std::unitbuf;
    net::io_context context;
    net::ip::tcp::endpoint ep(net::ip::address_v4::any(), 12345);
    net::ip::tcp::acceptor server(context, ep);
    demo::scope scope;

    scope.spawn([](auto& srv, auto& sc, auto sched)->demo::task<> {
        while (true) {
            std::cout << "awaiting a connetion\n";
            auto[client, addr] = co_await net::async_accept(srv);
            std::cout << "receiver connection from " << addr << "\n";
            sc.spawn(run_client(std::move(client), sched));
        }
    }(server, scope, context.get_scheduler()));
    
    context.run();
}
