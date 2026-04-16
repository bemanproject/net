// examples/postgres.cpp                                              -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <libpq-fe.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <memory>
#include <string>

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
}

int main() {
    std::cout << std::unitbuf;
    pq::connection conn(PQconnectdb("user=sruser dbname=sruser"));

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

    std::string input("/usr/share/dict/words");
    std::cout << "populating from '" << input << "'\n";
    std::ifstream file(input);
    std::string message;
    for (std::size_t i{}; i < 100 && std::getline(file, message); ++i) {
        std::ostringstream ins;
        ins << "insert into messages (key, message) values(" << i << ", '" << message << "');";
        std::cout << "inserting: " << ins.str() << '\n';
        pq::result res(PQexec(conn.get(), ins.str().c_str()));
        if (PQresultStatus(res.get()) != PGRES_COMMAND_OK) {
            std::cout << "INSERT failed: " << pq::error(conn) << '\n';
            return 1;
        }
    }
}
