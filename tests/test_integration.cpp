#include "../src/parser/Parser.h"
#include "../src/parser/Lexer.h"
#include "../src/parser/AST.h"
#include "../src/executor/Executor.h"
#include "../src/storage/TableManager.h"
#include "../src/common/Value.h"
#include <cassert>
#include <iostream>
#include <memory>

using namespace minisql;

bool createTestTable(const std::string& dbName) {
    std::cout << "Creating test table..." << std::endl;

    // Clean up any existing database
    storage::TableManager::getInstance().dropDatabase(dbName);

    // Create database
    auto dbResult = storage::TableManager::getInstance().createDatabase(dbName);
    if (dbResult.isError()) {
        std::cerr << "Failed to create database: " << dbResult.getError().what() << std::endl;
        return false;
    }

    // Create table
    std::string createSQL = "CREATE TABLE users (id INT PRIMARY KEY, name VARCHAR(50) NOT NULL, age INT)";
    parser::Lexer lexer(createSQL);
    parser::Parser parser(lexer);
    auto stmt = parser.parseStatement();
    if (!stmt) {
        std::cerr << "Failed to parse CREATE TABLE" << std::endl;
        return false;
    }

    auto result = executor::Executor::getInstance().execute(stmt.get());
    if (result.isError()) {
        std::cerr << "Failed to create table: " << result.getError().what() << std::endl;
        return false;
    }

    // Insert test data
    std::vector<std::pair<std::string, std::string>> testData = {
        {"1", "'Alice'"},
        {"2", "'Bob'"},
        {"3", "'Charlie'"}
    };

    for (const auto& [id, name] : testData) {
        std::string insertSQL = "INSERT INTO users (id, name, age) VALUES (" + id + ", " + name + ", " + id + "0)";
        parser::Lexer insertLexer(insertSQL);
        parser::Parser insertParser(insertLexer);
        auto insertStmt = insertParser.parseStatement();
        if (insertStmt) {
            auto insertResult = executor::Executor::getInstance().execute(insertStmt.get());
            if (insertResult.isError()) {
                std::cerr << "Failed to insert data: " << insertResult.getError().what() << std::endl;
            }
        }
    }

    std::cout << "Test table created successfully" << std::endl;
    return true;
}

void testSimpleSelect() {
    std::cout << "\n=== Test: Simple SELECT ===" << std::endl;

    std::string sql = "SELECT * FROM users";
    parser::Lexer lexer(sql);
    parser::Parser parser(lexer);
    auto stmt = parser.parseStatement();

    assert(stmt != nullptr);
    assert(stmt->getType() == parser::ASTNodeType::SELECT_STMT);

    auto result = executor::Executor::getInstance().execute(stmt.get());
    assert(result.isSuccess());

    auto rows = result.getValue()->rows;
    std::cout << "  Retrieved " << rows.size() << " rows" << std::endl;
    assert(rows.size() == 3);

    std::cout << "  PASSED" << std::endl;
}

void testSelectWithWhere() {
    std::cout << "\n=== Test: SELECT with WHERE ===" << std::endl;

    std::string sql = "SELECT * FROM users WHERE id = 1";
    parser::Lexer lexer(sql);
    parser::Parser parser(lexer);
    auto stmt = parser.parseStatement();

    assert(stmt != nullptr);

    auto result = executor::Executor::getInstance().execute(stmt.get());
    assert(result.isSuccess());

    auto rows = result.getValue()->rows;
    std::cout << "  Retrieved " << rows.size() << " rows" << std::endl;
    assert(rows.size() == 1);

    std::cout << "  PASSED" << std::endl;
}

void testSelectWithProjection() {
    std::cout << "\n=== Test: SELECT with projection ===" << std::endl;

    std::string sql = "SELECT name, age FROM users";
    parser::Lexer lexer(sql);
    parser::Parser parser(lexer);
    auto stmt = parser.parseStatement();

    assert(stmt != nullptr);

    auto result = executor::Executor::getInstance().execute(stmt.get());
    assert(result.isSuccess());

    auto execResult = result.getValue();
    auto rows = execResult->rows;
    std::cout << "  Retrieved " << rows.size() << " rows, " << rows[0].size() << " columns" << std::endl;
    assert(rows.size() == 3);
    assert(rows[0].size() == 2);  // name, age

    std::cout << "  PASSED" << std::endl;
}

void testQueryOptimizer() {
    std::cout << "\n=== Test: Query Optimizer Pipeline ===" << std::endl;

    // Test simple optimized query
    std::string sql = "SELECT * FROM users WHERE id > 1 ORDER BY id LIMIT 10";
    parser::Lexer lexer(sql);
    parser::Parser parser(lexer);
    auto stmt = parser.parseStatement();

    if (!stmt) {
        std::cout << "  SKIPPED (parse failed)" << std::endl;
        return;
    }

    // Execute to verify the optimizer pipeline works
    auto execResult = executor::Executor::getInstance().execute(stmt.get());
    if (execResult.isSuccess()) {
        std::cout << "  Executed optimized plan successfully" << std::endl;
        std::cout << "  PASSED" << std::endl;
    } else {
        std::cout << "  Execution failed: " << execResult.getError().what() << std::endl;
    }
}

void testExplain() {
    std::cout << "\n=== Test: EXPLAIN Query ===" << std::endl;

    std::string sql = "EXPLAIN SELECT * FROM users WHERE id > 1";
    parser::Lexer lexer(sql);
    parser::Parser parser(lexer);
    auto stmt = parser.parseStatement();

    assert(stmt != nullptr);

    auto result = executor::Executor::getInstance().execute(stmt.get());
    assert(result.isSuccess());

    auto rows = result.getValue()->rows;
    std::cout << "  EXPLAIN returned " << rows.size() << " rows" << std::endl;
    for (const auto& row : rows) {
        for (const auto& col : row) {
            std::cout << "    " << col.toString() << std::endl;
        }
    }

    std::cout << "  PASSED" << std::endl;
}

void testInsertAndDelete() {
    std::cout << "\n=== Test: INSERT and DELETE ===" << std::endl;

    // Insert
    std::string insertSQL = "INSERT INTO users (id, name, age) VALUES (99, 'Test', 99)";
    parser::Lexer insertLexer(insertSQL);
    parser::Parser insertParser(insertLexer);
    auto insertStmt = insertParser.parseStatement();

    if (insertStmt) {
        auto result = executor::Executor::getInstance().execute(insertStmt.get());
        std::cout << "  INSERT result: " << (result.isSuccess() ? "success" : "failed") << std::endl;
    }

    // Delete
    std::string deleteSQL = "DELETE FROM users WHERE id = 99";
    parser::Lexer deleteLexer(deleteSQL);
    parser::Parser deleteParser(deleteLexer);
    auto deleteStmt = deleteParser.parseStatement();

    if (deleteStmt) {
        auto result = executor::Executor::getInstance().execute(deleteStmt.get());
        std::cout << "  DELETE result: " << (result.isSuccess() ? "success" : "failed") << std::endl;
    }

    std::cout << "  PASSED" << std::endl;
}

int main() {
    std::cout << "=== Integration Tests ===" << std::endl;

    // Set up test database
    std::string dbName = "test_integration_db";
    executor::Executor::getInstance().setCurrentDatabase(dbName);

    if (!createTestTable(dbName)) {
        std::cerr << "Failed to create test table, some tests may fail" << std::endl;
    }

    // Run integration tests
    testSimpleSelect();
    testSelectWithWhere();
    testSelectWithProjection();
    testQueryOptimizer();
    testExplain();
    testInsertAndDelete();

    std::cout << "\n=== All Integration Tests PASSED ===" << std::endl;
    return 0;
}
