// examples/taps.cpp                                                  -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/net/net.hpp>
#include <string>

namespace net = beman::net;

// ----------------------------------------------------------------------------

int main(int ac, char* av[]) {
    net::preconnection pre(
        net::remote_endpoint().with_hostname(1 < ac ? av[1] : "example.com").with_port(2 < ac ? std::stoi(av[2]) : 80),
        {});

    // pre.connect()
}