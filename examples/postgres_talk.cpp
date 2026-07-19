// examples/postgres_talk.cpp                                         -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/net.hpp>
#include <beman/execution.hpp>
#include <libpq-fe.h>
#include "demo_algorithm.hpp"
#include "demo_http.hpp"
#include <chrono>
#include <iostream>
#include <optional>
#include <ranges>
#include <string>
#include <thread>
#include <vector>

namespace ex  = beman::execution;
namespace net = beman::net;
using namespace std::chrono_literals;

// PQconnectdb(const char* connstr) -> PGconn*
// PQfinish(PGconn*) - clean-up
// PQexec(const PGconn *conn, const char* query) -> PGresult* - query the database
// PQsendQuery(const PGconn *conn, const char* query) - send a query
// PQconsumeInput(const PGconn *conn) - consume available input, clear socket stat
// PQgetResult(const PGconn *conn) -> PGresult* - get result, return nullptr if no more results, or would block
// PQclear(PGresult) - clean-up
// PQsetnonblocking(const PGconn *conn, int arg) - set non-blocking mode to avoid write blocks
// PQsocket(const PGconn *conn) - get socket
// PQflush(const PGconn *conn) - flush output buffer, return 1 if still pending data
// PQisBusy(const PGconn *conn) -> int - PQgetResult() would block
// PQsetSingleRowMode(PGconn *conn) - set single row mode, return 0 on failure
// PQsetChunkedMode(PGconn *conn, int arg) - set chunked mode, return 0 on failure

namespace {
const std::string connection_string("user=sruser dbname=sruser");
const std::string query("select *, pg_sleep(0.5) from messages where 0 < key and key < 5;");
const std::string query2("select *, pg_sleep(0.5) from messages where 4 < key and key < 8;");

inline constexpr auto print_result [[maybe_unused]]{[](const PGresult* result) noexcept {
    std::cout << "n=" << PQntuples(result) << ", m=" << PQnfields(result) << "\n";
    for (int i = 0, n = PQntuples(result); i < n; ++i) {
        for (int j = 0, m = PQnfields(result); j < m; ++j == m || std::cout << ", ") {
            std::cout << PQgetvalue(result, i, j);
        }
        std::cout << "\n";
    }
}};
} // namespace

namespace pg {
struct conn {
    std::unique_ptr<PGconn, decltype([](auto c) { PQfinish(c); })> con;
    net::ip::tcp::socket                                           socket;
    conn(net::io_context& io, PGconn* c) : con(c), socket(io, io.make_socket(PQsocket(con.get()))) {}
    operator PGconn*() const { return con.get(); }
};

struct result {
    std::unique_ptr<PGresult, decltype([](auto r) { PQclear(r); })> res;
    result(PGresult* r) : res(r) {}
    operator PGresult*() const { return res.get(); }
};
using results = std::vector<pg::result>;
ex::task<results> exec(auto& conn, std::string query) {
    if (!PQsendQuery(conn, query.c_str())) {
        std::cout << "ERROR: " << PQerrorMessage(conn) << "\n";
        throw std::runtime_error("ERROR");
    }
    while (PQflush(conn)) {
        co_await net::async_poll(conn.socket, net::event_type::out);
    }
    pg::results res;
    std::cout << "sent query\n";

    while (true) {
        while (PQisBusy(conn)) {
            co_await ex::unstoppable(net::async_poll(conn.socket, net::event_type::in));
            if (!PQconsumeInput(conn)) {
                // error handling
            }
        }
        std::cout << "got a result\n";
        if (!res.emplace_back(PQgetResult(conn))) {
            res.pop_back();
            break;
        }
    }
    co_return res;
}

template <typename Object>
struct mutex {
    struct state_base {
        state_base*  next{};
        virtual void run() = 0;
    };

    Object      obj;
    bool        is_busy{false};
    state_base* waiting{};

    template <typename... A>
    mutex(A&&... a) : obj(std::forward<A>(a)...) {}

    template <typename Fun>
    using sender_t = decltype(std::declval<Fun>()(std::declval<Object&>()));

    template <typename Rcvr, typename Fun>
    struct state : state_base {
        using operation_state_concept = ex::operation_state_tag;
        std::remove_cvref_t<Rcvr> rcvr;
        mutex*                    mut;

        struct receiver {
            using receiver_concept = ex::receiver_tag;
            state* s;
            auto   get_env() const noexcept { return ex::get_env(s->rcvr); }
            template <typename... A>
            void set_value(A&&... a) noexcept {
                s->complete();
                ex::set_value(std::move(s->rcvr), std::forward<A>(a)...);
            }
            template <typename A>
            void set_error(A&& a) noexcept {
                s->complete();
                ex::set_error(std::move(s->rcvr), std::forward<A>(a));
            }
            void set_stopped() noexcept {
                s->complete();
                ex::set_stopped(std::move(s->rcvr));
            }
        };

        using inner_state_t = ex::connect_result_t<sender_t<Fun>, receiver>;

        inner_state_t inner_state;
        state(Rcvr&& r, Fun f, mutex* m)
            : rcvr(std::forward<Rcvr>(r)), mut(m), inner_state(ex::connect(std::move(f)(mut->obj), receiver{this})) {}
        void start() noexcept {
            if (!std::exchange(this->mut->is_busy, true)) {
                run();
            } else {
                this->next = std::exchange(mut->waiting, this);
            }
        }
        void complete() {
            if (mut->waiting) {
                std::exchange(mut->waiting, mut->waiting->next)->run();
            } else {
                mut->is_busy = false;
            }
        }
        void run() { ex::start(inner_state); }
    };

    template <typename Fun>
    struct sender {
        using sender_concept = ex::sender_tag;
        template <typename, typename... Env>
        static consteval auto get_completion_signatures() {
            return ex::get_completion_signatures<sender_t<Fun>, Env...>();
        }

        Fun    fun;
        mutex* mut;

        template <ex::receiver Rcvr>
        auto connect(Rcvr&& r) && {
            return state<Rcvr, Fun>(std::forward<Rcvr>(r), std::move(fun), mut);
        }
    };

    template <typename Fun>
    auto run(Fun fun) {
        return sender<Fun>(std::move(fun), this);
    }
};

auto exec(pg::mutex<pg::conn>& conn, std::string query) {
    return conn.run([=](pg::conn& obj) { return exec(obj, query); });
}
} // namespace pg

// ----------------------------------------------------------------------------

auto main() -> int {
    std::cout << std::unitbuf << "Postgres Example\n";
    net::io_context     io;
    pg::mutex<pg::conn> conn(io, PQconnectdb(connection_string.c_str()));

    auto clock = [](auto sched) -> ex::task<> {
        while (true) {
            std::cout << "time=" << std::chrono::system_clock::now() << "\n";
            // std::this_thread::sleep_for(1s);
            co_await net::resume_after(sched, 1s);
        }
        co_return;
    }(io.get_scheduler());
    std::cout << "do more!\n";

    auto query = [](auto& conn) -> ex::task<> {
        auto res = co_await pg::exec(conn, ::query);
        std::ranges::for_each(res, ::print_result);
        co_return;
    }(conn);
    auto query2 = [](auto& conn) -> ex::task<> {
        auto res = co_await pg::exec(conn, ::query2);
        std::ranges::for_each(res, ::print_result);
        co_return;
    }(conn);

    ex::sync_wait(demo::when_any(io.async_run(), std::move(clock), ex::when_all(std::move(query), std::move(query2))));
}
