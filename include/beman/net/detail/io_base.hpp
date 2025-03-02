// include/beman/net/detail/io_base.hpp                             -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef INCLUDED_BEMAN_NET_DETAIL_IO_BASE
#define INCLUDED_BEMAN_NET_DETAIL_IO_BASE

#include <beman/net/detail/netfwd.hpp>
#include <beman/net/detail/event_type.hpp>
#include <memory>
#include <system_error>
#include <ostream>

// ----------------------------------------------------------------------------

namespace beman::net::detail {
enum class submit_result { ready, submit, error };
auto operator<<(::std::ostream&, ::beman::net::detail::submit_result) -> ::std::ostream&;

struct io_base;
template <typename>
struct io_operation;
} // namespace beman::net::detail

// ----------------------------------------------------------------------------

inline auto beman::net::detail::operator<<(::std::ostream& out, ::beman::net::detail::submit_result result)
    -> ::std::ostream& {
    switch (result) {
    case ::beman::net::detail::submit_result::ready:
        return out << "ready";
    case ::beman::net::detail::submit_result::submit:
        return out << "submit";
    case ::beman::net::detail::submit_result::error:
        return out << "error";
    }
    return out << "<unknown>";
}

// ----------------------------------------------------------------------------
// The struct io_base is used as base class of operation states. Objects of
// this type are also used to kick off the actual work once a readiness
// indication was received.

struct beman::net::detail::io_base {
    using extra_t = ::std::unique_ptr<void, auto (*)(void*)->void>;
    using work_t  = auto (*)(::beman::net::detail::context_base&, io_base*) -> ::beman::net::detail::submit_result;

    io_base*                            next{nullptr}; // used for an intrusive list
    ::beman::net::detail::context_base* context{nullptr};
    ::beman::net::detail::socket_id     id;    // the entity affected
    ::beman::net::event_type            event; // mask for expected events
    work_t                              work;
    extra_t                             extra{nullptr, +[](void*) {}};

    io_base(::beman::net::detail::socket_id i, ::beman::net::event_type ev) : id(i), event(ev) {}
    virtual ~io_base() = default;

    virtual auto complete() -> void               = 0;
    virtual auto error(::std::error_code) -> void = 0;
    virtual auto cancel() -> void                 = 0;
};

// ----------------------------------------------------------------------------
// The struct io_operation is an io_base storing operation specific data.

template <typename Data>
struct beman::net::detail::io_operation : io_base, Data {
    template <typename D = Data>
    io_operation(::beman::net::detail::socket_id i, ::beman::net::event_type ev, D&& a = Data())
        : io_base(i, ev), Data(::std::forward<D>(a)) {}
};

// ----------------------------------------------------------------------------

#endif
