// tests/beman/sequence/connector.pass.cpp                            -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/sequence/detail/connector.hpp>
#include <type_traits>

namespace ex = beman::execution;
namespace sq = beman::sequence;
namespace sd = sq::detail;

// ----------------------------------------------------------------------------

namespace {
    struct receiver {
        using receiver_concept = ex::receiver_tag;
        int& value;
        void set_value(int v) && noexcept {
            this->value = 3 * v;
        }
    };
    static_assert(ex::receiver<receiver>);

    template <ex::receiver Rcvr>
    struct state {
        using operation_state_concept = ex::operation_state_tag;
        std::remove_cvref_t<Rcvr> rcvr;
        int value{};
        void start() & noexcept {
            ex::set_value(std::move(this->rcvr), 2 * this->value);
        }
    };
    static_assert(ex::operation_state<state<receiver>>);

    struct sender {
        using sender_concept = ex::sender_tag;
        template <typename...>
        static consteval auto get_completion_signatures() {
            return ex::completion_signatures<ex::set_value_t(int)>{};
        }

        int value{};

        template <ex::receiver Rcvr>
        state<Rcvr> connect(Rcvr&& rcvr) const {
            return {std::forward<Rcvr>(rcvr), this->value};
        }
    };
    static_assert(ex::sender<sender>);
}

int main() {
    int result{};
    sd::connector conn(sender{17}, receiver{result});

    assert(result == 0);
    ex::start(conn);
    assert(result == 102);
}
