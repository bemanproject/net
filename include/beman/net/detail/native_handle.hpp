// include/beman/net/detail/native_handle.hpp                         -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef INCLUDED_INCLUDE_BEMAN_NET_DETAIL_NATIVE_HANDLE
#define INCLUDED_INCLUDE_BEMAN_NET_DETAIL_NATIVE_HANDLE

#include <beman/net/detail/netfwd.hpp>
#include <fcntl.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

namespace beman::net::detail {

// ----------------------------------------------------------------------------

using native_poll_record = ::pollfd;

inline constexpr auto is_invalid_handle(native_handle_type handle) -> bool { return static_cast<int>(handle) < 0; }
inline constexpr auto is_valid_handle(native_handle_type handle) -> bool { return !::beman::net::detail::is_invalid_handle(handle); }

inline auto make_socket(int domain, int type, int protocol) -> native_handle_type  {
    return native_handle_type{::socket(domain, type, protocol)};
}
inline auto close_socket(native_handle_type handle) -> int {
    return ::close(static_cast<int>(handle));
}

inline auto set_socket_option(native_handle_type handle, int level, int name, const void* data, ::socklen_t size) -> int {
    return ::setsockopt(static_cast<int>(handle), level, name, data, size);
}

inline auto get_socket_option(native_handle_type handle, int level, int name, void* data, ::socklen_t* size) -> int {
    return ::getsockopt(static_cast<int>(handle), level, name, data, size);
}

inline auto bind(native_handle_type handle, const ::sockaddr* addr, ::socklen_t size) {
    return ::bind(static_cast<int>(handle), addr, size);
}

inline auto accept(native_handle_type handle, ::sockaddr* addr, ::socklen_t* size) {
    return native_handle_type{::accept(static_cast<int>(handle), addr, size)};
}

inline auto connect(native_handle_type handle, const ::sockaddr* addr, ::socklen_t size) ->int {
        return ::connect(static_cast<int>(handle), addr, size);
}

inline auto listen(native_handle_type handle, int no) {
    return ::listen(static_cast<int>(handle), no);
}

using native_event_type = decltype(native_poll_record{}.events);
inline auto make_native_poll_record(native_handle_type handle, native_event_type events, native_event_type revents)
{
    return native_poll_record{static_cast<int>(handle), events, revents};
}

inline auto file_control(native_handle_type handle, int command, auto... args) {
    return ::fcntl(static_cast<int>(handle), command, args...);
}

inline auto send_message(native_handle_type handle, const msghdr* message, int flags) -> ::ssize_t {
    return ::sendmsg(static_cast<int>(handle), message, flags);

}

inline auto receive_message(native_handle_type handle, msghdr* message, int flags) -> ::ssize_t {
    return ::recvmsg(static_cast<int>(handle), message, flags);

}

// ----------------------------------------------------------------------------

}

#endif
