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
using native_nfds_t    = unsigned long;
using native_msghdr    = WSAMSG;
using native_ssize_t   = int;

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

inline auto set_msg_iov(native_msghdr& msg, auto value) -> void {
#ifdef _MSC_VER
    msg.lpBuffers = value;
#else
    msg.msg_iov = value;
#endif
}
inline auto get_msg_iovlen(const native_msghdr& msg) {
#ifdef _MSC_VER
    return msg.dwBufferCount;
#else
    return msg.msg_iovlen;
#endif
}

inline auto set_msg_iovlen(native_msghdr& msg, auto value) -> void {
#ifdef _MSC_VER
    msg.dwBufferCount = value;
#else
    msg.msg_iovlen = value;
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
inline auto close_socket(native_handle_type handle) -> int {
#ifdef _MSC_VER
    return ::closesocket(handle);
#else
    return ::close(::beman::net::detail::native_handle_extract(handle));
#endif
}

inline auto set_socket_option(native_handle_type                     handle,
                              int                                    level,
                              int                                    name,
                              const void*                            data,
                              ::beman::net::detail::native_socklen_t size) -> int {
    return ::setsockopt(
        ::beman::net::detail::native_handle_extract(handle), level, name, static_cast<const char*>(data), size);
}

inline auto get_socket_option(
    native_handle_type handle, int level, int name, void* data, ::beman::net::detail::native_socklen_t* size) -> int {
    return ::getsockopt(
        ::beman::net::detail::native_handle_extract(handle), level, name, static_cast<char*>(data), size);
}

inline auto bind(native_handle_type handle, const ::sockaddr* addr, ::beman::net::detail::native_socklen_t size) {
    return ::bind(::beman::net::detail::native_handle_extract(handle), addr, size);
}

inline auto accept(native_handle_type handle, ::sockaddr* addr, ::beman::net::detail::native_socklen_t* size) {
    return native_handle_type{::accept(::beman::net::detail::native_handle_extract(handle), addr, size)};
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

inline auto set_nonblocking(native_handle_type handle) {
#ifdef _MSC_VER
    unsigned long flag(1);
    return ::ioctlsocket(handle, FIONBIO, &flag);
#else
    return ::fcntl(::beman::net::detail::native_handle_extract(handle), F_SETFL, O_NONBLOCK);
#endif
}

inline auto send_message(native_handle_type handle, ::beman::net::detail::native_msghdr* message, int flags)
    -> ::beman::net::detail::native_ssize_t {
#ifdef _MSC_VER
    DWORD sent{};
    if (SOCKET_ERROR == WSASend(handle, message->lpBuffers, message->dwBufferCount, &sent, flags, nullptr, nullptr)) {
        return -1;
    }
    return sent;
#else
    return ::sendmsg(::beman::net::detail::native_handle_extract(handle), message, flags);
#endif
}

inline auto receive_message(native_handle_type handle, ::beman::net::detail::native_msghdr* message, int flags)
    -> ::beman::net::detail::native_ssize_t {
#ifdef _MSC_VER
    DWORD received{};
    DWORD dFlags(flags);
    if (SOCKET_ERROR ==
        WSARecv(handle, message->lpBuffers, message->dwBufferCount, &received, &dFlags, nullptr, nullptr)) {
        return -1;
    }
    return received;
#else
    return ::recvmsg(::beman::net::detail::native_handle_extract(handle), message, flags);
#endif
}

inline auto poll(::beman::net::detail::native_poll_record* data, ::beman::net::detail::native_nfds_t size, int timeout)
    -> int {
#ifdef _MSC_VER
    return ::WSAPoll(data, size, timeout);
#else
    return ::poll(data, size, timeout);
#endif
}

// ----------------------------------------------------------------------------

} // namespace beman::net::detail

#endif
