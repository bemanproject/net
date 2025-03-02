// include/beman/net/detail/execution.hpp                           -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef INCLUDED_INCLUDE_BEMAN_NET_DETAIL_EXECUTION
#define INCLUDED_INCLUDE_BEMAN_NET_DETAIL_EXECUTION

#include <beman/execution/execution.hpp>

// ----------------------------------------------------------------------------

namespace beman::net::detail::ex::detail {
using ::beman::execution::detail::forward_like;
using ::beman::execution::detail::sender_adaptor;
using ::beman::execution::detail::type_list;
using ::beman::execution::detail::variant_or_empty;
using ::beman::execution::detail::meta::combine;
using ::beman::execution::detail::meta::filter;
using ::beman::execution::detail::meta::transform;
using ::beman::execution::detail::meta::unique;
} // namespace beman::net::detail::ex::detail

namespace beman::net::detail::ex {
using ::beman::execution::completion_signatures;
using ::beman::execution::detail::decayed_tuple;

using ::beman::execution::empty_env;
using ::beman::execution::env_of_t;
using ::beman::execution::error_types_of_t;
using ::beman::execution::get_env;
using ::beman::execution::value_types_of_t;

using ::beman::execution::get_completion_scheduler;
using ::beman::execution::get_completion_scheduler_t;
using ::beman::execution::get_completion_signatures;
using ::beman::execution::get_completion_signatures_t;
using ::beman::execution::get_delegation_scheduler;
using ::beman::execution::get_delegation_scheduler_t;
using ::beman::execution::get_scheduler;
using ::beman::execution::get_scheduler_t;
using ::beman::execution::get_stop_token;
using ::beman::execution::get_stop_token_t;

using ::beman::execution::operation_state;
using ::beman::execution::operation_state_t;
using ::beman::execution::receiver;
using ::beman::execution::receiver_t;
using ::beman::execution::scheduler;
using ::beman::execution::scheduler_t;
using ::beman::execution::sender;
using ::beman::execution::sender_t;

using ::beman::execution::sender_in;

using ::beman::execution::set_error;
using ::beman::execution::set_error_t;
using ::beman::execution::set_stopped;
using ::beman::execution::set_stopped_t;
using ::beman::execution::set_value;
using ::beman::execution::set_value_t;

using ::beman::execution::connect;
using ::beman::execution::connect_t;
using ::beman::execution::start;
using ::beman::execution::start_t;

using ::beman::execution::just;
using ::beman::execution::just_error;
using ::beman::execution::just_stopped;
using ::beman::execution::read_env;
using ::beman::execution::sync_wait;
using ::beman::execution::then;
using ::beman::execution::upon_error;
using ::beman::execution::upon_stopped;
using ::beman::execution::detail::write_env;
} // namespace beman::net::detail::ex

// ----------------------------------------------------------------------------

#endif
