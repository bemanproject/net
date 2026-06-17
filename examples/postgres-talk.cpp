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

} // namespace pg

// ----------------------------------------------------------------------------

auto main() -> int {
    std::cout << std::unitbuf << "Postgres Example\n";
}
