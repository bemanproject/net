// include/beman/net/detail/platform.hpp                              -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// ----------------------------------------------------------------------------

#ifndef INCLUDED_INCLUDE_BEMAN_NET_DETAIL_PLATFORM
#define INCLUDED_INCLUDE_BEMAN_NET_DETAIL_PLATFORM

// ----------------------------------------------------------------------------

#ifdef _MSC_VER
#    define NOMINMAX
#    include <WinSock2.h>
#    include <Windows.h>
#    include <ws2tcpip.h>
#else
#    include <unistd.h>
#    include <arpa/inet.h>
#    include <netinet/in.h>
#    include <poll.h>
#    include <fcntl.h>
#    include <sys/types.h>
#    include <sys/socket.h>
#endif

// ----------------------------------------------------------------------------

#endif
