// include/beman/net/detail/merge_completion_signatures.hpp           -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef INCLUDED_INCLUDE_BEMAN_NET_DETAIL_MERGE_COMPLETION_SIGNATURES
#define INCLUDED_INCLUDE_BEMAN_NET_DETAIL_MERGE_COMPLETION_SIGNATURES

// ----------------------------------------------------------------------------

namespace beman::net::detail {
    template <typename, typename> struct merge_completion_signatures_unique;
    template <typename S0>
    struct merge_completion_signatures_unique<S0, ::beman::execution::completion_signatures<>> {
        using type = S0;
    };
    template <typename... S0, typename S1>
    struct merge_completion_signatures_unique<
        ::beman::execution::completion_signatures<S0...>,
        ::beman::execution::completion_signatures<S1>
        > {
        using type = std::conditional_t<false,
            ::beman::execution::completion_signatures<S0...>,
            ::beman::execution::completion_signatures<S0..., S1>
            >;
    };
    template <typename S0, typename S1, typename... S2>
    struct merge_completion_signatures_unique<
        S0,
        ::beman::execution::completion_signatures<S1, S2...>
        > {
        using type = typename merge_completion_signatures_unique<
            typename merge_completion_signatures_unique<S0, ::beman::execution::completion_signatures<S1>>::type,
            ::beman::execution::completion_signatures<S2...>
            >::type;
    };

    template <typename...> struct merge_completion_signatures_helper;
    template <typename S0, typename S1>
    struct merge_completion_signatures_helper<S0, S1>
    {
        using type = typename merge_completion_signatures_unique<S0, S1>::type;
    };
    template <typename S0, typename S1, typename S2, typename... Sigs>
    struct merge_completion_signatures_helper<S0, S1, S2, Sigs...> {
        using type = typename merge_completion_signatures_helper<
            typename merge_completion_signatures_helper<S0, S1>::type,
            S2,
            Sigs...>::type;
    };

    template <typename... Sigs>
    inline consteval auto merge_completion_signatures() {
        return typename merge_completion_signatures_helper<Sigs...>::type();
    }
}

// ----------------------------------------------------------------------------

#endif
