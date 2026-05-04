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
#include <iostream>
#include <string>

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

auto exec2(pg::connection& conn, const char* query) {
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
ex::task<pg::result, pg::env> exec(pg::connection& conn, const char* query) {
    PQsendQuery(conn, query);
    while (PQflush(conn)) {
        co_await net::async_poll(conn.socket, net::event_type::out);
    }
    while (PQisBusy(conn)) {
        co_await net::async_poll(conn.socket, net::event_type::in);
        if (!PQconsumeInput(conn)) {
            co_yield ex::with_error(pg::error(PQerrorMessage(conn)));
        }
    }
    pg::result res(PQgetResult(conn));
    if (!res) {
        co_yield ex::with_error(pg::error(PQerrorMessage(conn)));
    }
    co_return std::move(res);
}
} // namespace pg

// ----------------------------------------------------------------------------

auto main() -> int {
    std::cout << std::unitbuf << "Postgres Example\n";
    net::io_context    io;
    pg::connection     conn(io, PQconnectdb(connection_string.c_str()));
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

    auto request{pg::exec2(conn, query.c_str()) | ex::then(print_result) |
                 ex::upon_error([](pg::error e) noexcept { std::cout << "database error=" << e << "\n"; })};

    if constexpr (false) {
        spawn(std::move(timer));
        spawn(std::move(request) | ex::then([&scope] noexcept { scope.request_stop(); }));
        ex::sync_wait(ex::when_all(io.async_run(), scope.join()));
    } else if constexpr (false) {
        ex::inplace_stop_source source;
        ex::sync_wait(ex::when_all(
            io.async_run(),
            ex::starts_on(io.get_scheduler(),
                          ex::write_env(std::move(timer), ex::env{ex::prop{ex::get_stop_token, source.get_token()}})),
            std::move(request) | ex::then([&source] noexcept { source.request_stop(); })));
    } else {
        ex::sync_wait(
            demo::when_any(io.async_run(), std::move(request), ex::starts_on(io.get_scheduler(), std::move(timer))));
    }
}
