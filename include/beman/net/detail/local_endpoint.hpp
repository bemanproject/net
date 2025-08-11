// include/beman/net/detail/local_endpoint.hpp                        -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef INCLUDED_INCLUDE_BEMAN_NET_DETAIL_LOCAL_ENDPOINT
#define INCLUDED_INCLUDE_BEMAN_NET_DETAIL_LOCAL_ENDPOINT

#include <string>

// ----------------------------------------------------------------------------

namespace beman::net::detail {
class local_endpoint;
}

class beman::net::detail::local_endpoint {
  public:
    local_endpoint() = default;

    auto hostname() const -> std::string { return this->_hostname; }
    auto with_hostname(const std::string& hostname) -> local_endpoint& {
        this->_hostname = hostname;
        return *this;
    }
    auto ip_address() const -> std::string { return this->_ip_address; }
    auto with_ip_address(const std::string& ip_address) -> local_endpoint& {
        this->_ip_address = ip_address;
        return *this;
    }
    auto port() const -> std::uint16_t { return this->_port; }
    auto with_port(std::uint16_t port) -> local_endpoint& {
        this->_port = port;
        return *this;
    }
    auto service() const -> std::string { return this->_service; }
    auto with_service(const std::string& service) -> local_endpoint& {
        this->_service = service;
        return *this;
    }
    auto interface() const -> std::string { return this->_interface; }
    auto with_interface(const std::string& interface) -> local_endpoint& {
        this->_interface = interface;
        return *this;
    }

  private:
    std::string   _hostname;
    std::string   _ip_address{};
    std::uint16_t _port{};
    std::string   _service{};
    std::string   _interface;
};

// ----------------------------------------------------------------------------

#endif
