// examples/print_completions.cpp                                     -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

template <typename>
struct print_completion_signatures_t;
template <typename... Signatures>
struct print_completion_signatures_t<ex::completion_signatures<Signatures...>> {
    void operator()(std::ostream& os) const { ((os << typeid(Signatures).name() << ','), ...); }
};
template <typename T>
inline constexpr print_completion_signatures_t<T> print_completion_signatures{};

struct print_completions_t {
    template <ex::sender Sender>
    struct sender {
        using sender_concept = ex::sender_tag;
        template <typename, typename... Env>
        static consteval auto get_completion_signatures() noexcept {
            return ex::get_completion_signatures<Sender, Env...>();
        }

        template <typename Receiver>
        struct state {
            using operation_state_concept = ex::operation_state_tag;
            using state_t                 = ex::connect_result_t<Sender, Receiver&&>;
            using env_t                   = ex::env_of_t<Receiver>;

            state_t state_;
            state(Receiver&& r, Sender&& s)
                : state_(ex::connect(std::forward<Sender>(s), std::forward<Receiver>(r))) {}
            auto start() noexcept -> void {
                std::cout << "completion_signatures<";
                print_completion_signatures<decltype(ex::get_completion_signatures<Sender, env_t>())>(std::cout);
                std::cout << ">\n";
                ex::start(this->state_);
            }
        };

        std::remove_cvref_t<Sender> sender;
        template <typename Receiver>
        auto connect(Receiver&& r) && {
            return state<Receiver>{std::forward<Receiver>(r), std::move(sender)};
        }
    };

    template <typename Sender>
    auto operator()(Sender&& sndr) const {
        return sender<Sender>{std::forward<Sender>(sndr)};
    }
};

[[maybe_unused]] inline constexpr print_completions_t print_completions{};
