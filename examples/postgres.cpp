// examples/postgres.cpp                                              -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/execution/execution.hpp>
#include <beman/net/net.hpp>
#include <libpq-fe.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <memory>
#include <string>

namespace ex = beman::execution;
namespace net = beman::net;

namespace pq {
    using connection = std::unique_ptr<PGconn, decltype([](auto conn){ PQfinish(conn); })>;
    using result = std::unique_ptr<PGresult, decltype([](auto res){ PQclear(res); })>;

    struct error {
        std::string msg;
        explicit error(const char* m) : msg(m) {}
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
    // PQsetSingleRowMode(PGconn *conn) - set single row mode, return 0 on failure
    // PQsetChunkedMode(PGconn *conn, int arg) - set chunked mode, return 0 on failure

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
                std::cout << "exec.start()\n";
                if (!PQsendQuery(conn.get(), query.c_str())) {
                    std::cout << "PQsendQuery failed: " << PQerrorMessage(conn.get()) << "\n";
                    ex::set_error(std::move(receiver), pq::error(PQerrorMessage(conn.get())));
                    return;
                }

                if (false && PQisBusy(conn.get())) {
                    ex::set_error(std::move(receiver), pq::error("PQsendQuery would block"));
                }
                else {
                    complete();
                }
            }
            void complete() noexcept {
                if (pq::result res{pq::result(PQgetResult(conn.get()))}) {
                    ex::set_value(std::move(receiver), std::move(res));
                }
                else {
                    ex::set_error(std::move(receiver), pq::error(conn));
                }
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
    std::cout << "connecting\n";
    pq::connection conn(PQconnectdb("user=sruser dbname=sruser"));
    PQsetnonblocking(conn.get(), 1);
    std::cout << "connection created\n";

    if (PQstatus(conn.get()) != CONNECTION_OK) {
        std::cout << "Connection to database failed: " << pq::error(conn) << '\n'; ;
        return 1;
    }
    [[maybe_unused]] const char*const query_version = "SELECT version(), pg_sleep(3)";
#if 0
    pq::result res(PQexec(conn.get(), query_version));
    if (PQresultStatus(res.get()) != PGRES_TUPLES_OK) {
        std::cout << "SELECT failed: " << pq::error(conn) << '\n';
        return 1;
    }
    std::cout << "PostgreSQL version: " << PQgetvalue(res.get(), 0, 0) << '\n';
#endif

    std::string query(
        "select *, pg_sleep(0.1) from messages where 0 <= key and key < 3;"
        "select *, pg_sleep(0.1) from messages where 3 <= key and key < 6;"
        );
    try {
        [[maybe_unused]] auto result = ex::sync_wait(pq::exec(conn, query));
        if (result) {
            auto[res] = *std::move(result);
            if (PQresultStatus(res.get()) != PGRES_TUPLES_OK) {
                std::cout << "SELECT failed: " << pq::error(conn) << '\n';
                return 1;
            }
            for (int i{}, n{PQntuples(res.get())}; i < n; ++i) {
                for (int j{}, m{PQnfields(res.get())}; j < m; ++j) {
                    std::cout << PQgetvalue(res.get(), i, j) << ',';
                }
                std::cout << '\n';
            }
        }
    }
    catch (const pq::error& e) {
        std::cout << "Error: " << e << '\n';
    }
}
