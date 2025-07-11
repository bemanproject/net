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

auto main() -> int {
    net::io_context context;
    net::ip::tcp::endpoint ep(net::ip::address_v4::any(), 12345);
    net::ip::tcp::acceptor server(context, ep);

    ex::sync_wait(net::async_accept(server));

    context.run();
}
