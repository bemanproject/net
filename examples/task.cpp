// examples/server.cpp     -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <iostream>
#include <functional>
#include <string_view>
#include <beman/execution/execution.hpp>
#include <beman/net/net.hpp>
#include "demo_scope.hpp"
#include "demo_task.hpp"

namespace ex  = ::beman::execution;
namespace net = ::beman::net;

namespace {
struct result {
    int value{};
};
struct error {
    int value{};
};
} // namespace

int main(int ac, char*[]) {
    try {
        auto res{ex::sync_wait(::std::invoke(
            [](int argc) -> demo::task<result> {
                int i = co_await ex::just(17);
                std::cout << "i=" << i << "\n";
                auto [a, b] = co_await ex::just("hello", "world");
                std::cout << "a=" << a << ", b=" << b << "\n";
                try {
                    co_await ex::just_error(error{17});
                } catch (const error& e) {
                    std::cout << "error=" << e.value << "\n";
                }
                if (argc == 2)
                    co_await ex::just_stopped();
                if (argc == 3)
                    throw error{42};
                co_return result{17};
            },
            ac))};

        if (res) {
            auto [r] = *res;
            std::cout << "after coroutine: r=" << r.value << "\n";
        } else {
            std::cout << "after coroutine: cancelled\n";
        }
    } catch (const error& e) {
        ::std::cout << "after coroutine: error=" << e.value << "\n";
    } catch (const std::exception& e) {
        ::std::cout << "after coroutine: exception=" << e.what() << "\n";
    } catch (...) {
        ::std::cout << "after coroutine: unknown exception\n";
    }
}
