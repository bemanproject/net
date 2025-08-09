// include/beman/net/detail/task.hpp                                  -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef INCLUDED_INCLUDE_BEMAN_NET_DETAIL_TASK
#define INCLUDED_INCLUDE_BEMAN_NET_DETAIL_TASK

#include <beman/execution/execution.hpp>
#include <beman/execution/task.hpp>
#include <beman/net/detail/get_io_handle.hpp>
#include <beman/net/detail/get_scope_token.hpp>
#include <beman/net/detail/io_context.hpp>
#include <beman/net/detail/scope.hpp>
#include <system_error>

// ----------------------------------------------------------------------------

namespace beman::net::detail {
class task_env {
  public:
    using error_types = beman::execution::completion_signatures<beman::execution::set_error_t(std::error_code)>;

    auto query(const beman::net::get_io_handle_t&) const noexcept { return this->_handle; }
    auto query(const beman::net::get_scope_token_t&) const noexcept { return this->_token; }

    task_env(const auto& env) : _handle(beman::net::get_io_handle(env)), _token(beman::net::get_scope_token(env)) {}
    task_env(task_env&&) = delete;

  private:
    beman::net::io_context::handle _handle;
    beman::net::scope::token       _token;
};

template <typename T = void>
using task = beman::execution::task<T, task_env>;
} // namespace beman::net::detail
namespace beman::net {
template <typename T = void>
using task = beman::net::detail::task<T>;
}

// ----------------------------------------------------------------------------

#endif
