// tests/beman/sequence/iota.pass.cpp                                 -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/sequence/detail/iota.hpp>
#include <beman/sequence/detail/ignore_all.hpp>
#include <beman/execution/execution.hpp>
#include <cassert>

namespace ex = beman::execution;
namespace sq = beman::sequence;

// ----------------------------------------------------------------------------

namespace {
template <ex::sender Sndr>
struct test_sender {
    using sender_concept = sq::sequence_sender_tag;
    template <typename, typename... E>
    static consteval auto get_completion_signatures() {
        return ex::get_completion_signatures<Sndr, E...>();
    }

    template <sq::sequence_receiver Rcvr>
    struct receiver {
        using receiver_concept = sq::sequence_receiver_tag;
        std::remove_cvref_t<Rcvr> rcvr;
        int&                      sum;
        template <typename... A>
        void set_value(A&&... a) && noexcept {
            ex::set_value(std::move(this->rcvr), std::forward<A>(a)...);
        }
        template <ex::sender Snd>
        auto set_next(Snd&& snd) noexcept {
            return std::forward<Snd>(snd) | ex::then([this](int v) noexcept { this->sum += v; });
        }
    };

    std::remove_cvref_t<Sndr> sndr;
    int&                      sum;
    template <ex::receiver Rcvr>
    auto sequence_connect(Rcvr&& rcvr) && {
        return sq::sequence_connect(std::move(sndr), receiver<Rcvr>(std::forward<Rcvr>(rcvr), this->sum));
    }
};
template <sq::sequence_sender Sndr>
test_sender(Sndr&&, int&) -> test_sender<Sndr>;

static_assert(ex::sender<test_sender<decltype(ex::just(17))>>);
static_assert(ex::sender_in<test_sender<decltype(ex::just(17))>>);
static_assert(sq::sequence_sender<test_sender<decltype(ex::just(17))>>);
} // namespace

int main() {
    auto sndr = sq::iota(1, 7);
    static_assert(ex::sender<decltype(sndr)>);
    static_assert(ex::sender_in<decltype(sndr)>);
    static_assert(sq::sequence_sender<decltype(sndr)>);

    int sum{};
    assert(sum == 0);
    ex::sync_wait(sq::ignore_all(test_sender{std::move(sndr), sum}));
    assert(sum == 21);
}
