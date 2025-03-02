// examples/demo_error.hpp                                            -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef INCLUDED_EXAMPLES_DEMO_ERROR
#define INCLUDED_EXAMPLES_DEMO_ERROR

#include <string>
#include <system_error>

// ----------------------------------------------------------------------------

namespace demo {
inline constexpr int timeout{1};

inline auto category() -> const std::error_category& {
    struct category : std::error_category {
        auto name() const noexcept -> const char* override { return "demo-category"; }
        auto message(int c) const noexcept -> std::string override { return c == 1 ? "timeout" : "unknown"; }
    };
    static category rc{};
    return rc;
}
} // namespace demo

// ----------------------------------------------------------------------------

#endif
