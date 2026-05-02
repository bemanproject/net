// examples/memory-stream.cpp                                         -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include "demo_stream.hpp"
#include <beman/net/net.hpp>
#include <beman/net/detail/repeat_effect_until.hpp>
#include <beman/execution/execution.hpp>
#include <libpq-fe.h>
#include "demo_algorithm.hpp"
#include "demo_http.hpp"
#include <chrono>
#include <iostream>
#include <string>

namespace ex  = ::beman::execution;
namespace net = ::beman::net;

// ----------------------------------------------------------------------------

int main() {
    std::cout << std::unitbuf;
    demo::context context;
    context.add("foo", [data=std::string_view("hello, world!")](demo::memory_base& s) mutable {
        std::size_t n{s.add_receive_data(data.substr(0, std::min(std::size_t(2), data.size())))};
        if (n == 0u) {
            return true;
        }
        data = data.substr(n);
        return false;
    });
    auto reader{[](demo::context& context)->ex::task<> {
        demo::memory mem(context, "foo");
        std::array<char, 10> buffer;
        while (std::size_t n = co_await mem.receive(net::buffer(buffer))) {
            std::cout << "stream received=" << std::string_view(buffer.begin(), n) << "\n";
        }
    }(context)};

   ex::sync_wait(ex::when_all(context.async_run(), std::move(reader)));
}
