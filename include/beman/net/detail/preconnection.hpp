// include/beman/net/detail/preconnection.hpp                         -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef INCLUDED_INCLUDE_BEMAN_NET_DETAIL_PRECONNECTION
#define INCLUDED_INCLUDE_BEMAN_NET_DETAIL_PRECONNECTION

#include <beman/net/detail/local_endpoint.hpp>
#include <beman/net/detail/remote_endpoint.hpp>
#include <beman/net/detail/security_props.hpp>
#include <beman/net/detail/transport_props.hpp>

// ----------------------------------------------------------------------------

namespace beman::net::detail {
class preconnection;
}
namespace beman::net {
using preconnection = beman::net::detail::preconnection;
}

// ----------------------------------------------------------------------------

class beman::net::detail::preconnection {
  public:
    preconnection(const remote_endpoint& re, const transport_props& tp = {}, const security_props& sp = {})
        : _remote(re), _local(), _transport_props(tp), _security_props(sp) {
        (void)this->_remote;
        (void)this->_local;
        (void)this->_transport_props;
        (void)this->_security_props;
    }

  private:
    remote_endpoint _remote;
    local_endpoint  _local;
    transport_props _transport_props;
    security_props  _security_props;
};

// ----------------------------------------------------------------------------

#endif
