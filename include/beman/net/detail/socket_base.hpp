// include/beman/net/detail/socket_base.hpp                         -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef INCLUDED_BEMAN_NET_DETAIL_SOCKET_BASE
#define INCLUDED_BEMAN_NET_DETAIL_SOCKET_BASE

// ----------------------------------------------------------------------------

#include <beman/net/detail/netfwd.hpp>
#include <sys/socket.h>

// ----------------------------------------------------------------------------

class beman::net::socket_base {
  public:
    template <typename Value, int Level, int Name>
    class socket_option {
      private:
        Value d_value;

      public:
        explicit socket_option(Value v) : d_value(v) {}
        Value value() const { return this->d_value; }
        template <typename Protocol>
        auto data(Protocol&&) const -> const Value* {
            return &this->d_value;
        }
        template <typename Protocol>
        auto data(Protocol&&) -> const Value* {
            return &this->d_value;
        }
        template <typename Protocol>
        constexpr auto level(Protocol&&) const -> int {
            return Level;
        }
        template <typename Protocol>
        constexpr auto name(Protocol&&) const -> int {
            return Name;
        }
        template <typename Protocol>
        constexpr auto size(Protocol&&) const -> ::socklen_t {
            return sizeof(Value);
        }
    };
    class broadcast;
    class debug;
    class do_not_route;
    class keep_alive;
    class linger;
    class out_of_band_inline;
    class receive_buffer_size;
    class receive_low_watermark;
    class reuse_address : public socket_option<int, SOL_SOCKET, SO_REUSEADDR> {
      public:
        explicit reuse_address(bool value) : socket_option(value) {}
        explicit operator bool() const { return this->value(); }
    };
    class send_buffer_size;
    class send_low_watermark;

    using shutdown_type = int; //-dk:TODO
    static constexpr shutdown_type shutdown_receive{1};
    static constexpr shutdown_type shutdown_send{2};
    static constexpr shutdown_type shutdown_both{3};

    using wait_type = int; //-dk:TODO
    static constexpr wait_type wait_read{1};
    static constexpr wait_type wait_write{2};
    static constexpr wait_type wait_error{3};

    using message_flags = int; //-dk:TODO
    static constexpr message_flags message_peek{1};
    static constexpr message_flags message_out_of_band{2};
    static constexpr message_flags message_do_not_route{3};

    static constexpr int max_listen_connections{SOMAXCONN};

  protected:
    socket_base()  = default;
    ~socket_base() = default;
};

// ----------------------------------------------------------------------------

#endif
