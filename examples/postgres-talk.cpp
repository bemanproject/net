// examples/postgres-talk.cpp                                         -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

// examples/http-server.cpp                                           -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/net/net.hpp>
#include <beman/execution/execution.hpp>
#include <libpq-fe.h>
#include "demo_algorithm.hpp"
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

    void print_result(const PGresult* result) {
        std::cout << "n=" << PQntuples(result) << ", m=" << PQnfields(result) << "\n";
        for (int i=0, n=PQntuples(result); i < n; ++i)  {
            for (int j=0, m=PQnfields(result); j < m; ++j == m || std::cout << ", ")  {
                std::cout << PQgetvalue(result, i, j);
            }
            std::cout << "\n";
        }
    }
}

// ----------------------------------------------------------------------------

auto main() -> int {
    std::cout << std::unitbuf << "Postgres Example\n";
}
