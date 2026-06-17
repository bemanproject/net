// examples/postgres-talk.cpp                                         -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/net/net.hpp>
#include <beman/execution/execution.hpp>
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

inline constexpr auto print_result{[](const PGresult* result) noexcept {
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
    std::unique_ptr<PGconn, decltype([](auto c) { PQfinish(c); })> cn;
    net::ip::tcp::socket                                           socket;
    conn(net::io_context& io, PGconn* c) : cn(c), socket(io, io.make_socket(PQsocket(c))) {}
    operator PGconn*() const { return cn.get(); }
};

struct result {
    std::unique_ptr<PGresult, decltype([](auto r) { PQclear(r); })> res;
    result(auto r) : res(r) {}
    operator PGresult*() const { return res.get(); }
};
using results = std::vector<pg::result>;
struct error {
    std::string message;
    error(std::string m) : message(m) { std::cout << "error:" << m << "\n"; }
};

struct env {
    using error_types = ex::completion_signatures<ex::set_error_t(pg::error)>;
};

ex::task<pg::results, pg::env> exec(auto& conn, std::string query) {
    if (!PQsendQuery(conn, query.c_str())) {
        co_yield ex::with_error(pg::error(PQerrorMessage(conn)));
    }
    while (PQflush(conn)) {
        co_await ex::unstoppable(net::async_poll(conn.socket, net::event_type::out));
    }
    pg::results res;
    while (true) {
        while (PQisBusy(conn)) {
            co_await ex::unstoppable(net::async_poll(conn.socket, net::event_type::in));
            if (!PQconsumeInput(conn)) {
                co_yield ex::with_error(pg::error(PQerrorMessage(conn)));
            }
        }
        if (!res.emplace_back(pg::result(PQgetResult(conn)))) {
            res.pop_back();
            break;
        }
    }
    co_return std::move(res);
}

template <typename Object>
class mutex {
    struct state_base {
        state_base*  next{};
        virtual void run() = 0;
    };
    Object      obj;
    bool        is_busy{};
    state_base* waiting{};

    template <typename Fun>
    using sender_t = decltype(std::declval<Fun>()(std::declval<Object&>()));

    template <ex::receiver Rcvr, typename Fun>
    struct state : state_base {
        using operation_state_concept = ex::operation_state_tag;

        std::remove_cvref_t<Rcvr> rcvr;
        mutex*                    self;

        struct receiver {
            using receiver_concept = ex::receiver_tag;
            state* st;

            auto get_env() const noexcept { return ex::get_env(st->rcvr); }
            template <typename... A>
            void set_value(A&&... a) && noexcept {
                st->complete();
                ex::set_value(std::move(st->rcvr), std::forward<A>(a)...);
            }
            template <typename E>
            void set_error(E&& e) && noexcept {
                st->complete();
                ex::set_error(std::move(st->rcvr), std::forward<E>(e));
            }
            void set_stopped() && noexcept {
                st->complete();
                ex::set_stopped(std::move(st->rcvr));
            }
        };
        using inner_state_t = ex::connect_result_t<sender_t<Fun>, receiver>;
        inner_state_t inner_state;

        state(Rcvr&& r, mutex* s, Fun f)
            : rcvr(std::forward<Rcvr>(r)),
              self(s),
              inner_state(ex::connect(std::move(f)(self->obj), receiver(this))) {}
        void start() & noexcept {
            if (std::exchange(self->is_busy, true)) {
                this->next = std::exchange(self->waiting, this);
            } else {
                run();
            }
        }
        void run() override { ex::start(inner_state); }
        void complete() {
            if (self->waiting) {
                std::exchange(self->waiting, self->waiting->next)->run();
            } else {
                self->is_busy = false;
            }
        }
    };
    template <typename Fun>
    struct sender {
        using sender_concept = ex::sender_tag;
        template <typename, typename... Env>
        static consteval auto get_completion_signatures() {
            return ex::get_completion_signatures<sender_t<Fun>, Env...>();
        }
        mutex* self;
        Fun    fun;

        template <ex::receiver Rcvr>
        auto connect(Rcvr&& rcvr) && {
            return state<Rcvr, Fun>(std::forward<Rcvr>(rcvr), self, std::move(fun));
        }
    };

  public:
    template <typename... A>
    mutex(A&&... a) : obj(std::forward<A>(a)...) {}
    template <typename Fun>
    auto run(Fun fun) {
        return sender<Fun>(this, std::move(fun));
    }
};

auto exec(pg::mutex<pg::conn>& conn, std::string query) {
    return conn.run([query = std::move(query)](pg::conn& conn) { return pg::exec(conn, query); });
}
} // namespace pg

// ----------------------------------------------------------------------------

auto main() -> int {
    std::cout << std::unitbuf << "Postgres Example\n";
    net::io_context     io;
    pg::mutex<pg::conn> conn(io, PQconnectdb(connection_string.c_str()));

    auto clock{[](auto sched) -> ex::task<> {
        while (true) {
            std::cout << "time=" << std::chrono::system_clock::now() << "\n";
            co_await net::resume_after(sched, 1s);
        }
    }(io.get_scheduler())};

    auto query1{[](auto sched, auto& conn) -> ex::task<> {
        auto res(co_await pg::exec(conn, query));
        std::ranges::for_each(res, print_result);
        co_await net::resume_after(sched, 2s);
    }(io.get_scheduler(), conn)};
    auto query2(pg::exec(conn, ::query2) | ex::then([](auto res) { std::ranges::for_each(res, print_result); }));

    // ex::sync_wait(demo::when_any());
    // ex::sync_wait(ex::when_all());

    ex::sync_wait(
        demo::when_any(io.async_run(), std::move(clock), ex::when_all(std::move(query1), std::move(query2))));
}
