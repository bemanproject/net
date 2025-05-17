// include/beman/net/detail/buffer.hpp                              -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef INCLUDED_BEMAN_NET_DETAIL_BUFFER
#define INCLUDED_BEMAN_NET_DETAIL_BUFFER

#include <string>
#include <system_error>
#include <cassert>
#include <cstddef>

namespace beman::net::detail {
#ifdef _MSC_VER
using native_iovec = WSABUF;
inline auto make_iov(auto base, auto len) { return ::WSABUF{.len = ULONG(len), .buf = static_cast<char*>(base)}; }
#else
using native_iovec = ::iovec;
inline auto make_iov(auto base, auto len) { return ::iovec{.iov_base = base, .iov_len = len}; }
#endif
} // namespace beman::net::detail

// ----------------------------------------------------------------------------

namespace beman::net {
enum class stream_errc : int;

auto stream_category() noexcept -> const ::std::error_category&;

auto make_error_code(::beman::net::stream_errc) noexcept -> ::std::error_code;
auto make_error_condition(::beman::net::stream_errc) noexcept -> ::std::error_condition;

struct mutable_buffer;
struct const_buffer;

template <typename>
struct is_mutable_buffer_sequence;
template <typename>
struct is_const_buffer_sequence;
template <typename>
struct is_dynamic_buffer;

struct buffer_sequence;

template <::std::size_t _S>
auto buffer(char (&)[_S]) -> ::beman::net::mutable_buffer;
auto buffer(char*, ::std::size_t) -> ::beman::net::mutable_buffer;
auto buffer(const char*, ::std::size_t) -> ::beman::net::const_buffer;
template <typename _CT>
    requires requires(_CT&& _C) {
        _C.data();
        _C.size();
    }
auto buffer(_CT&& _C) {
    return beman::net::buffer(_C.data(), _C.size());
}
} // namespace beman::net

// ----------------------------------------------------------------------------

enum class beman::net::stream_errc : int { eof, not_found };

// ----------------------------------------------------------------------------

inline auto beman::net::stream_category() noexcept -> const ::std::error_category& {
    struct _Category : ::std::error_category {
        auto name() const noexcept -> const char* override { return "stream_error"; }
        auto message(int) const noexcept -> ::std::string override { return {}; }
    };
    static _Category _Rc{};
    return _Rc;
}

// ----------------------------------------------------------------------------

struct beman::net::mutable_buffer {
    ::beman::net::detail::native_iovec _Vec;
    mutable_buffer(void* _B, ::std::size_t _L) : _Vec(::beman::net::detail::make_iov(_B, _L)) {}

    auto data() -> ::beman::net::detail::native_iovec* { return &this->_Vec; }
    auto size() -> ::std::size_t { return 1u; }
};

struct beman::net::const_buffer {
    ::beman::net::detail::native_iovec _Vec;
    const_buffer(const void* _B, ::std::size_t _L) : _Vec(::beman::net::detail::make_iov(const_cast<void*>(_B), _L)) {}

    auto data() -> ::beman::net::detail::native_iovec* { return &this->_Vec; }
    auto size() -> ::std::size_t { return 1u; }
};

template <::std::size_t _S>
inline auto beman::net::buffer(char (&_B)[_S]) -> ::beman::net::mutable_buffer {
    return ::beman::net::mutable_buffer(_B, _S);
}

inline auto beman::net::buffer(char* _B, ::std::size_t _Size) -> ::beman::net::mutable_buffer {
    return ::beman::net::mutable_buffer(_B, _Size);
}

inline auto beman::net::buffer(const char* _B, ::std::size_t _Size) -> ::beman::net::const_buffer {
    return ::beman::net::const_buffer(_B, _Size);
}

// ----------------------------------------------------------------------------

#endif
