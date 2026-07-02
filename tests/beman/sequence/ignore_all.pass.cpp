// tests/beman/sequence/ignore_all.pass.cpp                           -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/sequence/detail/ignore_all.hpp>
#include <beman/sequence/detail/connector.hpp>
#include <beman/sequence/detail/set_next.hpp>
#include <beman/sequence/detail/sequence_sender.hpp>
#include <beman/execution/execution.hpp>
#include <iostream>
#if defined(NDBUG)
#   undef NDEBUG
#endif
#include <cassert>

namespace ex = beman::execution;
namespace sq = beman::sequence;

// ----------------------------------------------------------------------------

namespace {
    template <::beman::execution::receiver Rcvr>
    struct state {
        using operation_state_concept = ::beman::execution::operation_state_tag;
        using rcvr_t = ::std::remove_cvref_t<Rcvr>;

        rcvr_t rcvr;
        bool& started;
        bool& done;

        struct receiver {
            using receiver_concept = ::beman::execution::receiver_tag;
            state* st;

            void set_value() && noexcept {
                this->st->done = true;
                ::beman::execution::set_value(::std::move(this->st->rcvr));
            }
            auto set_next(::beman::execution::sender auto&&) & noexcept {
                return ::beman::sequence::set_next(this->st->rcvr, ::beman::execution::just());
            }
        };
        using sndr_t = decltype(::beman::sequence::set_next(::std::declval<receiver&>(), ::beman::execution::just()));

        ::std::optional<sq::detail::connector<sndr_t, receiver>> inner_state;

        void start() & noexcept {
            this->started = true;
            inner_state.emplace(::beman::sequence::set_next(this->rcvr, ::beman::execution::just()), receiver{this});
            ::beman::execution::start(inner_state->st);
        }
    };
    struct sender {
        using sender_concept = ::beman::sequence::sequence_sender_tag;
        template <typename, typename...>
        static consteval auto get_completion_signatures() noexcept {
            return ::beman::execution::completion_signatures<::beman::execution::set_value_t()>();
        }

        bool& started;
        bool& done;

        template <::beman::execution::receiver Rcvr>
        auto connect(Rcvr&& rcvr) const& noexcept {
            std::cout << "ignore_all.pass.cpp: connect() called\n";
            return state<Rcvr>{::std::forward<Rcvr>(rcvr), this->started, this->done};
        }
    };

    static_assert(ex::sender<sender>);
    static_assert(sq::sequence_sender<sender>);
}

int main() {
    bool started{false};
    bool done{false};
    static_assert(ex::sender<decltype(sq::ignore_all(sender{started, done}))>);
    static_assert(ex::sender_in<decltype(sq::ignore_all(sender{started, done}))>);
    static_assert(sq::sequence_sender<decltype(sq::ignore_all(sender{started, done}))>);

    std::cout << std::unitbuf;
    std::cout << "ignore_all.pass.cpp: started=" << std::boolalpha << started << ", done=" << done << '\n';
    assert(not started);
    assert(not done);
    ex::sync_wait(sq::ignore_all(sender{started, done}));
    assert(started);
    assert(done);
    std::cout << "ignore_all.pass.cpp: started=" << std::boolalpha << started << ", done=" << done << '\n';
}
