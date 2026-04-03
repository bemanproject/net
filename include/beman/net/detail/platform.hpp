// include/beman/net/detail/platform.hpp                        -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef INCLUDED_BEMAN_NET_DETAIL_PLATFORM
#define INCLUDED_BEMAN_NET_DETAIL_PLATFORM

// ----------------------------------------------------------------------------
// Platform abstraction for networking primitives.
// On Windows: provides WinSock2 headers and POSIX compatibility types.
// On POSIX: provides the standard socket and time headers.

#ifdef _WIN32

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>

#ifndef _SOCKLEN_T_DEFINED
#define _SOCKLEN_T_DEFINED
typedef int socklen_t;
#endif

// POSIX compatibility types not available on Windows
struct iovec {
    void*  iov_base;
    size_t iov_len;
};

struct msghdr {
    void*         msg_name;
    socklen_t     msg_namelen;
    struct iovec* msg_iov;
    int           msg_iovlen;
    void*         msg_control;
    size_t        msg_controllen;
    int           msg_flags;
};

#else // POSIX

#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#endif // _WIN32

// ----------------------------------------------------------------------------

#endif
