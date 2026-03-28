// tests/server/test_network.cpp
#include "../../src/server/network/ThreadPool.h"
#include "../../src/server/network/SqlProtocol.h"
#include <iostream>
#include <cassert>

using namespace minisql;

void testThreadPool() {
    ThreadPool pool(4);

    auto future1 = pool.submit([]() { return 1; });
    auto future2 = pool.submit([]() { return 2; });

    assert(future1.get() == 1);
    assert(future2.get() == 2);

    pool.stop();

    std::cout << "[PASS] ThreadPool test" << std::endl;
}

void testSqlProtocol() {
    SqlRequest request = SqlProtocol::parse("SELECT * FROM users;\n");
    assert(request.sql == "SELECT * FROM users;");

    SqlResponse response;
    response.success = true;
    response.message = "OK";
    response.rowCount = 2;
    response.columns = {"id", "name"};
    response.rows = {{"1", "Alice"}, {"2", "Bob"}};

    std::string resp = SqlProtocol::buildResponse(response);
    assert(resp.find("OK") != std::string::npos);
    assert(resp.find("id,name") != std::string::npos);

    std::cout << "[PASS] SqlProtocol test" << std::endl;
}

int main() {
    std::cout << "=== Network Test ===" << std::endl;

    testThreadPool();
    testSqlProtocol();

    std::cout << "=== All Tests Passed ===" << std::endl;
    return 0;
}
