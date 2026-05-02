// examples/memory-stream.cpp                                         -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include "demo_stream.hpp"
#include "demo_http.hpp"
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
    std::string   request("GET /some/url HTTP/1.1\r\nHost: localhost:12345\r\nUser-Agent: memory/0.0\r\nAccept: */*\r\n\r\n");
    context.add("foo", [data=std::string_view(request)](demo::memory_base& s) mutable {
        if (std::size_t n{s.add_receive_data(data.substr(0, std::min(std::size_t(2), data.size())))}) {
            data = data.substr(n);
            return false;
        }
        return true;
    });

    auto reader{[](auto client)->ex::task<> {
        co_await client.request();
    }(demo::http_client(demo::mem_stream(context, "foo")))};

   ex::sync_wait(ex::when_all(context.async_run(), std::move(reader)));
}
