// include/beman/net/detail/remote_endpoint.hpp                       -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef INCLUDED_INCLUDE_BEMAN_NET_DETAIL_REMOTE_ENDPOINT
#define INCLUDED_INCLUDE_BEMAN_NET_DETAIL_REMOTE_ENDPOINT

// ----------------------------------------------------------------------------

namespace beman::net::detail {
class remote_endpoint;
}
namespace beman::net {
using remote_endpoint = beman::net::detail::remote_endpoint;
}

class beman::net::detail::remote_endpoint {
  public:
    remote_endpoint() = default;

    auto hostname() const -> std::string { return this->_hostname; }
    auto with_hostname(const std::string& hostname) -> remote_endpoint& {
        this->_hostname = hostname;
        return *this;
    }
    auto ip_address() const -> std::string { return this->_ip_address; }
    auto with_ip_address(const std::string& ip_address) -> remote_endpoint& {
        this->_ip_address = ip_address;
        return *this;
    }
    auto port() const -> std::uint16_t { return this->_port; }
    auto with_port(std::uint16_t port) -> remote_endpoint& {
        this->_port = port;
        return *this;
    }
    auto service() const -> std::string { return this->_service; }
    auto with_service(const std::string& service) -> remote_endpoint& {
        this->_service = service;
        return *this;
    }

  private:
    std::string   _hostname;
    std::string   _ip_address;
    std::uint16_t _port{};
    std::string   _service{};
};

// ----------------------------------------------------------------------------

#endif
