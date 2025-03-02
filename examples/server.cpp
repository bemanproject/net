// examples/server.cpp     -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <iostream>
#include <functional>
#include <string_view>
#include <beman/execution/execution.hpp>
#include <beman/net/net.hpp>
#include "demo_algorithm.hpp"
#include "demo_scope.hpp"
#include "demo_task.hpp"

namespace ex  = ::beman::execution;
namespace net = ::beman::net;
using namespace std::chrono_literals;

auto use(auto&&) -> void {}

/// timeout(time, sender, ....)
///{
///     return when_any(sender..., resume_after(time) | into_error())
/// }

auto make_client(auto client) -> demo::task<void> {
    try {
        char buffer[8];
        while (auto size = co_await net::async_receive(client, net::buffer(buffer)))
        // while (auto size = co_await timeout(
        //     net::async_receive(client, net::buffer(buffer),
        //     resume_after(5s);
        //) | upon_error[](auto){ return 0; })))
        {
            std::string_view message(+buffer, size);
            std::cout << "received<" << size << ">(" << message << ")\n";
            auto ssize = co_await net::async_send(client, net::const_buffer(buffer, size));
            std::cout << "sent<" << ssize << "/" << message.size() << ">(" << ::std::string_view(buffer, ssize)
                      << ")\n";
        }
        std::cout << "client done\n";
    } catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << '\n';
    }
}

struct receiver {
    using receiver_concept = ex::receiver_t;
    auto set_error(auto&&) && noexcept -> void {}
    auto set_stopped() && noexcept -> void {}
    auto set_value(auto&&...) && noexcept -> void {}
};

int main() {
    std::cout << std::unitbuf;
    std::cout << "example server\n";

    try {
        demo::scope     scope;
        net::io_context context;

        scope.spawn(std::invoke(
            [](auto& scp, auto& ctxt) -> demo::task<> {
                net::ip::tcp::endpoint endpoint(net::ip::address_v4::any(), 12345);
                net::ip::tcp::acceptor acceptor(ctxt, endpoint);

                while (true) {
                    try {
                        auto [stream, ep] =
                            co_await demo::when_any(net::async_accept(acceptor),
                                                    demo::into_error(net::resume_after(ctxt.get_scheduler(), 1s),
                                                                     [](auto&&...) { return ::std::error_code(); }));
                        ::std::cout << "when_any is done\n";

                        std::cout << "ep=" << ep << "\n";
                        scp.spawn(make_client(std::move(stream)));
                    } catch (...) {
                        std::cout << "timer fired\n";
                    }
                }
            },
            scope,
            context));

        context.run();
    } catch (const std::exception& ex) {
        std::cout << "EXCEPTION: " << ex.what() << "\n";
        abort();
    }
}
