// examples/postgres-talk.cpp                                         -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

// examples/http-server.cpp                                           -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/net/net.hpp>
#include <beman/net/detail/repeat_effect_until.hpp>
#include <beman/execution/execution.hpp>
#include <libpq-fe.h>
#include "demo_algorithm.hpp"
#include "demo_http.hpp"
#include <chrono>
#include <optional>
#include <ranges>
#include <iostream>
#include <string>
#include <vector>

namespace ex  = beman::execution;
namespace net = beman::net;
using namespace std::chrono_literals;

// PQconnectdb(const char* connstr) -> PGconn*
// PQexec(const PGconn *conn, const char* query) -> PGresult* - query the database
// PQsendQuery(const PGconn *conn, const char* query) - send a query
// PQconsumeInput(const PGconn *conn) - consume available input, clear socket stat
// PQgetResult(const PGconn *conn) -> PGresult* - get result, return nullptr if no more results, or would block
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
struct connection {
    std::unique_ptr<PGconn, decltype([](auto c) { PQfinish(c); })> conn;
    net::ip::tcp::socket                                           socket;
    connection(net::io_context& io, PGconn* c)
        : conn(c ? c : throw std::runtime_error("connection failed")),
          socket(io, io.make_socket(PQsocket(conn.get()))) {}
    operator PGconn*() const { return conn.get(); }
};

template <typename Object>
struct limit {
    struct state_base {
        state_base*  next{};
        virtual void run() noexcept = 0;
    };
    Object      obj;
    bool        in_use{};
    state_base* head{};

    template <typename... Args>
    limit(Args&&... args) : obj(std::forward<Args>(args)...) {}

    template <typename Receiver, typename Fun>
    struct state : state_base {
        using operation_state_concept = ex::operation_state_tag;
        std::remove_cvref_t<Receiver> receiver;
        limit*                        self;
        Fun                           fun;

        struct rcvr {
            using receiver_concept = ex::receiver_tag;
            state* st;
            auto   get_env() const noexcept { return ex::get_env(st->receiver); }
            template <typename... A>
            void set_value(A&&... a) noexcept {
                st->complete();
                ex::set_value(std::move(st->receiver), std::forward<A>(a)...);
            }
            template <typename E>
            void set_error(E&& e) noexcept {
                st->complete();
                ex::set_error(std::move(st->receiver), std::forward<E>(e));
            }
            void set_stopped() noexcept {
                st->complete();
                ex::set_stopped(std::move(st->receiver));
            }
        };

        using inner_state_t = ex::connect_result_t<decltype(std::declval<Fun>()(std::declval<Object&>())), rcvr>;
        struct connector {
            inner_state_t st;
            template <ex::sender S, ex::receiver R>
            connector(S&& s, R&& r) : st(ex::connect(std::forward<S>(s), std::forward<R>(r))) {}
            void start() { ex::start(this->st); }
        };
        std::optional<connector> inner_state;

        state(Receiver&& r, limit* s, Fun f) : receiver(std::forward<Receiver>(r)), self(s), fun(std::move(f)) {}

        void start() & noexcept {
            if (std::exchange(this->self->in_use, true)) {
                this->next = std::exchange(this->self->head, this);
            } else {
                this->run();
            }
        }
        void complete() {
            if (this->self->head) {
                std::exchange(this->self->head, this->self->head->next)->run();
            } else {
                this->self->in_use = false;
            }
        }
        void run() noexcept override {
            this->inner_state.emplace(std::move(this->fun)(this->self->obj), rcvr{this});
            this->inner_state->start();
        }
    };
    template <typename Fun>
    struct sender {
        using sender_concept = ex::sender_tag;
        template <typename, typename... Env>
        static consteval auto get_completion_signatures() {
            return ex::get_completion_signatures<decltype(std::declval<Fun>()(std::declval<Object&>())), Env...>();
        }
        template <ex::receiver Receiver>
        auto connect(Receiver&& receiver) && noexcept {
            return state<Receiver, Fun>(std::forward<Receiver>(receiver), this->self, std::move(this->fun));
        }

        limit* self;
        Fun    fun;
    };
    template <typename Fun>
    auto operator()(Fun fun) {
        return sender<Fun>{this, std::move(fun)};
    }
};

struct error {
    std::string          msg;
    friend std::ostream& operator<<(std::ostream& out, const error& e) { return out << e.msg; }
};

struct env {
    using error_types = ex::completion_signatures<ex::set_error_t(pg::error)>;
};

struct result {
    std::unique_ptr<PGresult, decltype([](auto r) { PQclear(r); })> res;
    result(PGresult* r) : res(r) {}
    operator PGresult*() const noexcept { return res.get(); }
};

using results = std::vector<result>;
auto print_results([](const results& res) noexcept { std::ranges::for_each(res, print_result); });

auto exec1(pg::connection& conn, const char* query) {
    return ex::just() | ex::then([&conn, query] noexcept { PQsendQuery(conn, query); }) |
           net::repeat_effect_until(net::async_poll(conn.socket, net::event_type::out) |
                                        ex::upon_error([](auto&&) noexcept {}) | ex::then([](auto&&...) noexcept {}),
                                    [&conn] noexcept { return not PQflush(conn); }) |
           net::repeat_effect_until(net::async_poll(conn.socket, net::event_type::in) |
                                        ex::upon_error([](auto&&) noexcept {}) |
                                        ex::then([&conn](auto&&...) noexcept { PQconsumeInput(conn); }),
                                    [&conn] noexcept { return !PQisBusy(conn); }) |
           ex::then([&conn] noexcept { return pg::result(PQgetResult(conn)); });
}
ex::task<pg::results, pg::env> exec2(pg::connection& conn, const char* query) {
    PQsendQuery(conn, query);
    while (PQflush(conn)) {
        co_await net::async_poll(conn.socket, net::event_type::out);
    }
    pg::results res;
    while (true) {
        while (PQisBusy(conn)) {
            co_await net::async_poll(conn.socket, net::event_type::in);
            if (!PQconsumeInput(conn)) {
                co_yield ex::with_error(pg::error(PQerrorMessage(conn)));
            }
        }
        res.push_back(PQgetResult(conn));
        if (!res.back()) {
            res.pop_back();
            break;
        }
    }
    co_return std::move(res);
}

auto exec(const char* query) {
    return [query](pg::connection& conn) { return pg::exec2(conn, query); };
}
} // namespace pg

// ----------------------------------------------------------------------------

auto main() -> int {
    std::cout << std::unitbuf << "Postgres Example\n";
    net::io_context    io;
    pg::limit<pg::connection> conn(io, PQconnectdb(connection_string.c_str()));
    ex::counting_scope scope;
    auto               spawn{
        [&](ex::sender auto s) { ex::spawn(ex::starts_on(io.get_scheduler(), std::move(s)), scope.get_token()); }};

#if 0
    spawn(demo::http_server(io, 12345, [&spawn](auto client) noexcept {
        std::cout << "got a client\n";
        spawn([](auto client)->ex::task<void, demo::http::no_error_env>{
            std::cout << "reading request\n";
            co_await client.request();
            std::cout << "client done\n";
        }(std::move(client)));
    }));
#endif

    struct io_env {
        using scheduler_type = decltype(io.get_scheduler());
        using error_types    = ex::completion_signatures<>;
    };

    auto timer{[]() -> ex::task<void, io_env> {
        while (true) {
            std::cout << "time=" << std::chrono::system_clock::now() << "\n";
            co_await net::resume_after(co_await ex::read_env(ex::get_scheduler), 1s);
        }
    }()};

    auto request1{conn(pg::exec(query.c_str())) | ex::then(pg::print_results) |
                  ex::upon_error([](pg::error e) noexcept { std::cout << "database error=" << e << "\n"; })};
    auto request2{conn(pg::exec(query2.c_str())) | ex::then(pg::print_results) |
                  ex::upon_error([](pg::error e) noexcept { std::cout << "database error=" << e << "\n"; })};

    if constexpr (false) {
        spawn(std::move(timer));
        spawn(std::move(request1) | ex::then([&scope] noexcept { scope.request_stop(); }));
        ex::sync_wait(ex::when_all(io.async_run(), scope.join()));
    } else if constexpr (false) {
        ex::inplace_stop_source source;
        ex::sync_wait(ex::when_all(
            io.async_run(),
            ex::starts_on(io.get_scheduler(),
                          ex::write_env(std::move(timer), ex::env{ex::prop{ex::get_stop_token, source.get_token()}})),
            std::move(request1) | ex::then([&source] noexcept { source.request_stop(); })));
    } else {
        ex::sync_wait(
            demo::when_any(io.async_run(),
                           // std::move(request1) | ex::let_value([&request2]{ return std::move(request2); }),
                           ex::when_all(std::move(request1), std::move(request2)),
                           ex::starts_on(io.get_scheduler(), std::move(timer))));
    }
}
