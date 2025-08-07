// include/beman/net/detail/task.hpp                                  -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef INCLUDED_INCLUDE_BEMAN_NET_DETAIL_TASK
#define INCLUDED_INCLUDE_BEMAN_NET_DETAIL_TASK

#include <beman/execution/execution.hpp>
#include <beman/execution/task.hpp>
#include <beman/net/detail/io_context.hpp>

// ----------------------------------------------------------------------------

namespace beman::net::detail {
struct task_env {
    using error_types = beman::execution::completion_signatures<>;

    beman::net::io_context::handle _handle;
    auto query(beman::net::get_io_handle_t const&) const noexcept {
        return this->_handle;
    }

    task_env(auto const& env)
        : _handle(beman::net::get_io_handle(env)) {
    }

};

template <typename T>
using task = beman::execution::task<T, task_env>;
} // namespace beman::net::detail
namespace beman::net {
template <typename T>
using task = beman::net::detail::task<T>;
}

// ----------------------------------------------------------------------------

#endif
