// examples/sync_run.hpp                                              -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef INCLUDED_EXAMPLES_SYNC_RUN
#define INCLUDED_EXAMPLES_SYNC_RUN

struct sync_run_env {
    ex::run_loop& loop;
    auto          query(const ex::get_scheduler_t&) const noexcept { return this->loop.get_scheduler(); }
};
struct sync_run_receiver {
    ex::run_loop& loop;
    using receiver_concept = ex::receiver_t;

    auto get_env() const noexcept { return sync_run_env{this->loop}; }
    auto set_value() noexcept { this->loop.finish(); }
};
auto sync_run(ex::run_loop& loop, ex::sender auto snd) {
    auto state{ex::connect(std::move(snd), sync_run_receiver{loop})};
    ex::start(state);
    std::cout << "running loop\n";
    loop.run();
    std::cout << "running loop done\n";
}

// ----------------------------------------------------------------------------

#endif
