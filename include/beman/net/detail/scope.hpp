// include/beman/net/detail/scope.hpp                                 -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef INCLUDED_INCLUDE_BEMAN_NET_DETAIL_SCOPE
#define INCLUDED_INCLUDE_BEMAN_NET_DETAIL_SCOPE

#include <beman/net/detail/io_context.hpp>
#include <beman/net/detail/get_io_handle.hpp>
#include <beman/net/detail/get_scope_token.hpp>
#include <beman/execution/execution.hpp>

// ----------------------------------------------------------------------------

namespace beman::net::detail {
class scope;
}
namespace beman::net {
using scope = beman::net::detail::scope;
}

// ----------------------------------------------------------------------------

class beman::net::detail::scope {
  public:
    class token;
    class env {
        friend class token;

      public:
        auto query(const beman::net::get_io_handle_t&) const noexcept {
            return this->_scope->_io_context.get_handle();
        }
        auto query(const beman::execution::get_scheduler_t&) const noexcept {
            return this->_scope->_io_context.get_scheduler();
        }
        auto query(const beman::net::get_scope_token_t&) const noexcept { return this->_scope->get_token(); }

      private:
        env(scope* s) : _scope(s) {}
        scope* _scope;
    };
    class token {
      public:
        token(scope* s) : _scope(s), _counting_token(s->_counting_scope.get_token()) {}

        auto try_associate() noexcept -> bool { return this->_counting_token.try_associate(); }
        auto disassociate() noexcept -> void { this->_counting_token.disassociate(); }
        template <beman::execution::sender Sender, typename... Env>
        auto wrap(Sender&& sndr, const Env&... ev) const noexcept -> beman::execution::sender auto {
            return this->_counting_token.wrap(
                beman::execution::write_env(std::forward<Sender>(sndr), env(this->_scope)), ev...);
        }

      private:
        scope*                                  _scope;
        beman::execution::counting_scope::token _counting_token;
    };

    auto run() -> auto {
        return beman::execution::when_all(this->_counting_scope.join(), this->_io_context.async_run());
    }

    auto get_token() -> token { return {this}; }

  private:
    beman::net::io_context           _io_context;
    beman::execution::counting_scope _counting_scope;
};
static_assert(beman::execution::scope_token<beman::net::detail::scope::token>);

// ----------------------------------------------------------------------------

#endif
