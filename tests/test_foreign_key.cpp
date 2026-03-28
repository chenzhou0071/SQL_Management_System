#include "../src/parser/Parser.h"
#include "../src/parser/Lexer.h"
#include "../src/parser/AST.h"
#include "../src/executor/Executor.h"
#include "../src/storage/TableManager.h"
#include <cassert>
#include <iostream>
#include <memory>
#include <chrono>
#include <ctime>

using namespace minisql;

std::string getUniqueDbName() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    return "test_fk_" + std::to_string(time);
}

bool setupTestData(const std::string& dbName) {
    std::cout << "Setting up test data..." << std::endl;

    executor::Executor::getInstance().setCurrentDatabase(dbName);

    // Create parent table (users)
    std::string createUsers = "CREATE TABLE users (id INT PRIMARY KEY, name VARCHAR(50))";
    parser::Lexer usersLexer(createUsers);
    parser::Parser usersParser(usersLexer);
    auto usersStmt = usersParser.parseStatement();
    if (!usersStmt) return false;
    auto result = executor::Executor::getInstance().execute(usersStmt.get());
    if (result.isError()) return false;

    // Create child table (orders) with foreign key
    std::string createOrders = "CREATE TABLE orders (id INT PRIMARY KEY, user_id INT, amount INT, FOREIGN KEY (user_id) REFERENCES users(id))";
    parser::Lexer ordersLexer(createOrders);
    parser::Parser ordersParser(ordersLexer);
    auto ordersStmt = ordersParser.parseStatement();
    if (!ordersStmt) return false;
    auto ordersResult = executor::Executor::getInstance().execute(ordersStmt.get());
    if (ordersResult.isError()) return false;

    // Insert users
    std::vector<std::string> userIds = {"1", "2", "3"};
    for (const auto& id : userIds) {
        std::string sql = "INSERT INTO users (id, name) VALUES (" + id + ", 'User" + id + "')";
        parser::Lexer lex(sql);
        parser::Parser p(lex);
        auto stmt = p.parseStatement();
        if (stmt) executor::Executor::getInstance().execute(stmt.get());
    }

    std::cout << "Test data setup complete" << std::endl;
    return true;
}

void testForeignKeyViolation() {
    std::cout << "\n=== Test: Foreign Key Violation on INSERT ===" << std::endl;

    // Try to insert an order with a non-existent user_id (should fail)
    std::string sql = "INSERT INTO orders (id, user_id, amount) VALUES (1, 999, 100)";
    parser::Lexer lexer(sql);
    parser::Parser parser(lexer);
    auto stmt = parser.parseStatement();

    assert(stmt != nullptr);

    auto result = executor::Executor::getInstance().execute(stmt.get());
    if (result.isError()) {
        std::cout << "  Correctly rejected: " << result.getError().what() << std::endl;
        std::cout << "  PASSED" << std::endl;
    } else {
        std::cout << "  ERROR: Should have been rejected!" << std::endl;
        assert(false && "Foreign key violation not detected!");
    }
}

void testForeignKeyValid() {
    std::cout << "\n=== Test: Valid Foreign Key INSERT ===" << std::endl;

    // Insert an order with valid user_id (should succeed)
    std::string sql = "INSERT INTO orders (id, user_id, amount) VALUES (1, 1, 100)";
    parser::Lexer lexer(sql);
    parser::Parser parser(lexer);
    auto stmt = parser.parseStatement();

    assert(stmt != nullptr);

    auto result = executor::Executor::getInstance().execute(stmt.get());
    if (result.isSuccess()) {
        std::cout << "  Correctly accepted valid foreign key" << std::endl;
        std::cout << "  PASSED" << std::endl;
    } else {
        std::cout << "  ERROR: Should have been accepted: " << result.getError().what() << std::endl;
        assert(false);
    }
}

void testForeignKeyUpdateViolation() {
    std::cout << "\n=== Test: Foreign Key Violation on UPDATE ===" << std::endl;

    // Update an order to have invalid user_id (should fail)
    std::string sql = "UPDATE orders SET user_id = 888 WHERE id = 1";
    parser::Lexer lexer(sql);
    parser::Parser parser(lexer);
    auto stmt = parser.parseStatement();

    assert(stmt != nullptr);

    auto result = executor::Executor::getInstance().execute(stmt.get());
    if (result.isError()) {
        std::cout << "  Correctly rejected: " << result.getError().what() << std::endl;
        std::cout << "  PASSED" << std::endl;
    } else {
        std::cout << "  ERROR: Should have been rejected!" << std::endl;
        assert(false && "Foreign key violation not detected!");
    }
}

void testNoForeignKeyTable() {
    std::cout << "\n=== Test: Table Without Foreign Keys ===" << std::endl;

    // Insert into users table (no foreign keys) should work regardless
    std::string sql = "INSERT INTO users (id, name) VALUES (100, 'TestUser')";
    parser::Lexer lexer(sql);
    parser::Parser parser(lexer);
    auto stmt = parser.parseStatement();

    assert(stmt != nullptr);

    auto result = executor::Executor::getInstance().execute(stmt.get());
    if (result.isSuccess()) {
        std::cout << "  Correctly accepted insert without FK check failure" << std::endl;
        std::cout << "  PASSED" << std::endl;
    } else {
        std::cout << "  ERROR: " << result.getError().what() << std::endl;
        assert(false);
    }
}

int main() {
    std::cout << "=== Foreign Key Constraint Tests ===" << std::endl;

    std::string dbName = getUniqueDbName();
    storage::TableManager::getInstance().createDatabase(dbName);

    if (!setupTestData(dbName)) {
        std::cerr << "Failed to setup test data" << std::endl;
        return 1;
    }

    testForeignKeyViolation();
    testForeignKeyValid();
    testForeignKeyUpdateViolation();
    testNoForeignKeyTable();

    std::cout << "\n=== All Foreign Key Tests PASSED ===" << std::endl;
    return 0;
}
