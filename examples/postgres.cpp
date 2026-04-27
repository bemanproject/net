// examples/postgres.cpp                                              -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/execution/execution.hpp>
#include <beman/net/net.hpp>
#include <libpq-fe.h>
#include <chrono>
#include <fstream>
#include <iostream>
#include <sstream>
#include <memory>
#include <string>
#include <stdexcept>

namespace ex  = beman::execution;
namespace net = beman::net;
using namespace std::chrono_literals;

namespace pg {
struct connection {
    using handle_t = std::unique_ptr<PGconn, decltype([](auto conn) { PQfinish(conn); })>;

    handle_t             handle;
    net::ip::tcp::socket socket;
    connection(net::io_context& io, PGconn* conn)
        : handle(conn), socket(io.get_scheduler().get_context(), io.make_socket(PQsocket(handle.get()))) {
        if (PQstatus(conn) != CONNECTION_OK) {
            throw std::runtime_error(std::string("Connection to database failed: ") + PQerrorMessage(conn));
        }
    }
    connection(connection&&) noexcept = default;

    net::ip::tcp::socket& get_socket() { return this->socket; }
                          operator PGconn*() { return this->handle.get(); }
                          operator const PGconn*() const { return this->handle.get(); }
};

struct error {
    std::string msg;
    explicit error(const char* m) : msg(m) {}
    error(const connection& conn) : msg(PQerrorMessage(conn)) {}
    const char*          what() const noexcept { return msg.c_str(); };
    friend std::ostream& operator<<(std::ostream& os, const error& err) { return os << err.msg; }
};

using result = std::unique_ptr<PGresult, decltype([](auto res) { PQclear(res); })>;

// PQsetnonblocking(const PGconn *conn, int arg) - set non-blocking mode to avoid write blocks
// PQsocket(const PGconn *conn) - get socket
// PQflush(const PGconn *conn) - flush output buffer, return 1 if still pending data
// PQisBusy(const PGconn *conn) - PQgetResult() would block
// PQconsumeInput(const PGconn *conn) - consume available input, clear socket stat
// PQgetResult(const PGconn *conn) - get result, return nullptr if no more results, or would block
// PQsetSingleRowMode(PGconn *conn) - set single row mode, return 0 on failure
// PQsetChunkedMode(PGconn *conn, int arg) - set chunked mode, return 0 on failure

struct exec {
    using sender_concept        = ex::sender_t;
    using completion_signatures = ex::completion_signatures<ex::set_value_t(result), ex::set_error_t(error)>;
    template <typename...>
    static consteval completion_signatures get_completion_signatures() noexcept {
        return {};
    }

    template <typename Receiver>
    struct state {
        using operation_state_concept = ex::operation_state_t;

        struct env {
            using error_types = ex::completion_signatures<ex::set_error_t(error)>;
        };
        static ex::task<result, env> work([[maybe_unused]] connection& conn,
                                          [[maybe_unused]] std::string query) noexcept {
            std::cout << "exec.work()\n";
            if (!PQsendQuery(conn, query.c_str())) {
                std::cout << "PQsendQuery failed: " << PQerrorMessage(conn) << "\n";
                co_yield ex::with_error(pg::error(conn));
            }
            PQflush(conn);
            while (PQisBusy(conn)) {
                std::cout << "co_awaiting poll\n";
                auto evs = co_await net::async_poll(conn.get_socket(), net::event_type::in);
                std::cout << "co_awaiting done=" << evs << "\n";
                if (!PQconsumeInput(conn)) {
                    std::cout << "PQconsumeInput failed: " << PQerrorMessage(conn) << "\n";
                    co_yield ex::with_error(pg::error(conn));
                }
            }
            if (pg::result res{pg::result(PQgetResult(conn))}) {
                co_return std::move(res);
            } else {
                co_yield ex::with_error(pg::error(conn));
            }
            std::unreachable();
        }
        using inner_state_t =
            ex::connect_result_t<decltype(work(std::declval<connection&>(), std::string{})), Receiver&&>;

        inner_state_t inner_state;

        state(Receiver&& r, connection& conn, std::string query)
            : inner_state(ex::connect(work(conn, std::move(query)), std::forward<Receiver>(r))) {}

        auto start() noexcept -> void { ex::start(this->inner_state); }
    };

    template <ex::receiver Receiver>
    auto connect(Receiver&& receiver) && noexcept {
        return state<Receiver>{std::forward<Receiver>(receiver), conn, std::move(query)};
    }

    exec(connection& conn, std::string query) : conn(conn), query(std::move(query)) {}
    connection& conn;
    std::string query;
};

inline constexpr double sleep_time = 3.0;
} // namespace pg

namespace {
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
        using sender_concept = ex::sender_t;
        template <typename, typename... Env>
        static consteval auto get_completion_signatures() noexcept {
            return ex::get_completion_signatures<Sender, Env...>();
        }

        template <typename Receiver>
        struct state {
            using operation_state_concept = ex::operation_state_t;
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

} // namespace

int main() {
    std::cout << std::unitbuf << "PostgreSQL example\n";
    try {
        net::io_context    io;
        pg::connection     conn(io, PQconnectdb("user=sruser dbname=sruser"));
        ex::counting_scope scope;
        ex::run_loop       loop;

        auto spawn{[&](ex::sender auto s) {
            ex::spawn(ex::starts_on(loop.get_scheduler(), std::move(s)), scope.get_token());
        }};

        struct noexcept_env {
            using error_types = ex::completion_signatures<>;
        };

        spawn(ex::just());
        spawn(std::invoke(
            [](auto sched) noexcept -> ex::task<void, noexcept_env> {
                while (true) {
                    std::cout << "\rtime=" << std::chrono::system_clock::now() << "\n" << std::flush;
                    co_await net::resume_after(sched, 1s);
                }
            },
            io.get_scheduler()));

        std::string query("select *, pg_sleep(1.1) from messages where 0 <= key and key < 3;");

        spawn(pg::exec(conn, query) | ex::then([&](pg::result res, auto&&...) noexcept {
                  for (int i{}, n{PQntuples(res.get())}; i < n; ++i) {
                      for (int j{}, m{PQnfields(res.get())}; j < m; ++j) {
                          std::cout << PQgetvalue(res.get(), i, j) << ',';
                      }
                      std::cout << '\n';
                  }
                  std::cout << "query done\n" << std::flush;
                  scope.request_stop();
              }) |
              ex::upon_error([](pg::error error) noexcept { std::cout << "query error: " << error << "\n"; }));

        sync_run(loop, ex::when_all(scope.join(), io.async_run()) | ex::upon_stopped([]() noexcept {}));
    } catch (const pg::error& e) {
        std::cout << "Error: " << e << '\n';
    }
}
