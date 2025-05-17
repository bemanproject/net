// include/beman/net/detail/native_handle.hpp                         -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef INCLUDED_INCLUDE_BEMAN_NET_DETAIL_NATIVE_HANDLE
#define INCLUDED_INCLUDE_BEMAN_NET_DETAIL_NATIVE_HANDLE

#include <beman/net/detail/netfwd.hpp>
#ifndef _MSC_VER
#include <fcntl.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>
#endif
#include <cstddef>

namespace beman::net::detail {

// ----------------------------------------------------------------------------

#ifdef _MSC_VER
using native_socklen_t = int;
using native_nfds_t    = ::std::size_t;
struct native_msghdr {
    void*         msg_iov;
    ::std::size_t msg_iovlen;
};
using native_ssize_t = int;

#else
using native_socklen_t = ::socklen_t;
using native_nfds_t    = ::nfds_t;
using native_msghdr    = ::msghdr;
using native_ssize_t   = ::ssize_t;

#endif

inline auto native_handle_extract(native_handle_type handle) {
#ifdef _MSC_VER
    return handle;
#else
    return static_cast<int>(handle);
#endif
}
// ----------------------------------------------------------------------------

using native_poll_record = ::pollfd;

inline auto is_invalid_handle(native_handle_type handle) -> bool {
#ifdef _MSC_VER
    return handle == INVALID_SOCKET;
#else
    return ::beman::net::detail::native_handle_extract(handle) < 0;
#endif
}
inline auto is_valid_handle(native_handle_type handle) -> bool {
    return !::beman::net::detail::is_invalid_handle(handle);
}

inline auto make_socket(int domain, int type, int protocol) -> native_handle_type {
    return native_handle_type{::socket(domain, type, protocol)};
}
inline auto close_socket([[maybe_unused]] native_handle_type handle) -> int {
#ifdef _MSC_VER
    return {}; //-dk:TODO
#else
    return ::close(::beman::net::detail::native_handle_extract(handle));
#endif
}

inline auto set_socket_option([[maybe_unused]] native_handle_type                     handle,
                              [[maybe_unused]] int                                    level,
                              [[maybe_unused]] int                                    name,
                              [[maybe_unused]] const void*                            data,
                              [[maybe_unused]] ::beman::net::detail::native_socklen_t size) -> int {
#ifdef _MSC_VER
    return {}; //-dk:TODO
#else
    return ::setsockopt(::beman::net::detail::native_handle_extract(handle), level, name, data, size);
#endif
}

inline auto get_socket_option([[maybe_unused]] native_handle_type                      handle,
                              [[maybe_unused]] int                                     level,
                              [[maybe_unused]] int                                     name,
                              [[maybe_unused]] void*                                   data,
                              [[maybe_unused]] ::beman::net::detail::native_socklen_t* size) -> int {
#ifdef _MSC_VER
    return {}; //-dk:TODO
#else
    return ::getsockopt(::beman::net::detail::native_handle_extract(handle), level, name, data, size);
#endif
}

inline auto bind(native_handle_type handle, const ::sockaddr* addr, ::beman::net::detail::native_socklen_t size) {
    return ::bind(::beman::net::detail::native_handle_extract(handle), addr, size);
}

inline auto accept(native_handle_type handle, ::sockaddr* addr, ::beman::net::detail::native_socklen_t* size) {
#ifdef _MSC_VER
    return native_handle_type{::accept(handle, addr, size)};
#else
    return native_handle_type{::accept(::beman::net::detail::native_handle_extract(handle), addr, size)};
#endif
}

inline auto connect(native_handle_type handle, const ::sockaddr* addr, ::beman::net::detail::native_socklen_t size)
    -> int {
    return ::connect(::beman::net::detail::native_handle_extract(handle), addr, size);
}

inline auto listen(native_handle_type handle, int no) {
    return ::listen(::beman::net::detail::native_handle_extract(handle), no);
}

using native_event_type = decltype(native_poll_record{}.events);
inline auto make_native_poll_record(native_handle_type handle, native_event_type events, native_event_type revents) {
    return native_poll_record{::beman::net::detail::native_handle_extract(handle), events, revents};
}

inline auto file_control([[maybe_unused]] native_handle_type handle,
                         [[maybe_unused]] int                command,
                         [[maybe_unused]] auto... args) -> int {
#ifdef _MSC_VER
    return {}; //-dk:TODO
#else
    return ::fcntl(::beman::net::detail::native_handle_extract(handle), command, args...);
#endif
}

inline auto send_message([[maybe_unused]] native_handle_type                         handle,
                         [[maybe_unused]] const ::beman::net::detail::native_msghdr* message,
                         [[maybe_unused]] int flags) -> ::beman::net::detail::native_ssize_t {
#ifdef _MSC_VER
    return {}; //-dk:TODO
#else
    return ::sendmsg(::beman::net::detail::native_handle_extract(handle), message, flags);
#endif
}

inline auto receive_message([[maybe_unused]] native_handle_type                   handle,
                            [[maybe_unused]] ::beman::net::detail::native_msghdr* message,
                            [[maybe_unused]] int flags) -> ::beman::net::detail::native_ssize_t {
#ifdef _MSC_VER
    return {}; //-dk:TODO
#else
    return ::recvmsg(::beman::net::detail::native_handle_extract(handle), message, flags);
#endif
}

inline auto poll([[maybe_unused]] ::beman::net::detail::native_poll_record* data,
                 [[maybe_unused]] ::beman::net::detail::native_nfds_t       size,
                 [[maybe_unused]] int                                       timeout) -> int {
#ifdef _MSC_VER
    return {}; //-dk:TODO
#else
    return ::poll(data, size, timeout);
#endif
}

// ----------------------------------------------------------------------------

} // namespace beman::net::detail

#endif
