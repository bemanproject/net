// examples/postgres.cpp                                              -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/execution/execution.hpp>
#include <beman/net/net.hpp>
#include <libpq-fe.h>
#include <stdio.h>
#include <iostream>
#include <memory>
#include <string>

namespace ex = beman::execution;
namespace net = beman::net;

namespace pq {
    using connection = std::unique_ptr<PGconn, decltype([](auto conn){ PQfinish(conn); })>;
    using result = std::unique_ptr<PGresult, decltype([](auto res){ PQclear(res); })>;

    struct error {
        std::string msg;
        error(const connection& conn) : msg(PQerrorMessage(conn.get())) {}
        const char* what() const noexcept { return msg.c_str(); };
        friend std::ostream& operator<< (std::ostream& os, const error& err) {
            return os << err.msg;
        }
    };

    // PQsetnonblocking(const PGconn *conn, int arg) - set non-blocking mode to avoid write blocks
    // PQsocket(const PGconn *conn) - get socket
    // PQflush(const PGconn *conn) - flush output buffer, return 1 if still pending data
    // PQisBusy(const PGconn *conn) - PQgetResult() would block
    // PQconsumeInput(const PGconn *conn) - consume available input, clear socket stat
    // PQgetResult(const PGconn *conn) - get result, return nullptr if no more results, or would block

    struct exec {
        using sender_concept = ex::sender_t;
        using completion_signatures = ex::completion_signatures<ex::set_value_t(result), ex::set_error_t(error)>;
        template <typename...>
        static consteval completion_signatures get_completion_signatures() noexcept { return {}; }

        template <typename Receiver>
        struct state {
            using operation_state_concept = ex::operation_state_t;
            std::remove_cvref_t<Receiver> receiver;
            connection& conn;
            std::string query;
            auto start() noexcept -> void {
            }
        };

        template <ex::receiver Receiver>
        auto connect(Receiver&& receiver) {
            return state<Receiver>{std::forward<Receiver>(receiver), conn, std::move(query)};
        }

        exec(connection& conn, std::string query) : conn(conn), query(std::move(query)) {}
        connection& conn;
        std::string query;
    };

    inline constexpr double sleep_time = 3.0;
}

int main() {
    std::cout << std::unitbuf;
    net::io_context io;
    pq::connection conn(PQconnectdb("user=sruser dbname=srchat"));
    PQsetnonblocking(conn.get(), 1);

    if (PQstatus(conn.get()) != CONNECTION_OK) {
        std::cout << "Connection to database failed: " << pq::error(conn) << '\n'; ;
        return 1;
    }
    const char*const query_version = "SELECT version(), pg_sleep(0)";
    pq::result res(PQexec(conn.get(), query_version));
    if (PQresultStatus(res.get()) != PGRES_TUPLES_OK) {
        std::cout << "SELECT failed: " << pq::error(conn) << '\n';
        return 1;
    }
    std::cout << "PostgreSQL version: " << PQgetvalue(res.get(), 0, 0) << '\n';

    [[maybe_unused]] auto result = ex::sync_wait(pq::exec(conn, query_version));
    if (result) {
        auto[res] = *std::move(result);
        if (PQresultStatus(res.get()) != PGRES_TUPLES_OK) {
            std::cout << "SELECT failed: " << pq::error(conn) << '\n';
            return 1;
        }
        std::cout << "PostgreSQL version: " << PQgetvalue(res.get(), 0, 0) << '\n';
    }
}
