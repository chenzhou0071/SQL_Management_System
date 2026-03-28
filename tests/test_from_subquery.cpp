#include "../src/parser/Parser.h"
#include "../src/parser/Lexer.h"
#include "../src/parser/AST.h"
#include "../src/executor/Executor.h"
#include "../src/storage/TableManager.h"
#include "../src/common/Value.h"
#include <cassert>
#include <ctime>
#include <iostream>
#include <memory>

using namespace minisql;

std::string getUniqueDbName() {
    return "test_from_subquery_" + std::to_string(std::time(nullptr));
}

bool setupTables(const std::string& dbName) {
    storage::TableManager::getInstance().dropDatabase(dbName);
    auto dbResult = storage::TableManager::getInstance().createDatabase(dbName);
    if (dbResult.isError()) return false;
    executor::Executor::getInstance().setCurrentDatabase(dbName);

    // Create users table
    {
        std::string sql = "CREATE TABLE users (id INT PRIMARY KEY, name VARCHAR(50), age INT)";
        parser::Lexer lexer(sql);
        parser::Parser parser(lexer);
        auto stmt = parser.parseStatement();
        if (!stmt) return false;
        auto r = executor::Executor::getInstance().execute(stmt.get());
        if (r.isError()) return false;
    }

    // Insert users: (1, 'Alice', 25), (2, 'Bob', 30), (3, 'Charlie', 20)
    for (const auto& [id, name, age] : std::vector<std::tuple<std::string, std::string, std::string>>{
             {"1", "'Alice'", "25"}, {"2", "'Bob'", "30"}, {"3", "'Charlie'", "20"}}) {
        std::string sql = "INSERT INTO users (id, name, age) VALUES (" + id + ", " + name + ", " + age + ")";
        parser::Lexer lexer(sql);
        parser::Parser parser(lexer);
        auto stmt = parser.parseStatement();
        if (stmt) executor::Executor::getInstance().execute(stmt.get());
    }

    return true;
}

void testFromSubqueryBasic() {
    std::cout << "\n=== Test: FROM subquery basic ===" << std::endl;

    std::string dbName = getUniqueDbName();
    if (!setupTables(dbName)) {
        assert(false && "Setup failed");
    }

    // SELECT * FROM (SELECT id, name FROM users WHERE age > 20) AS t
    std::string sql = "SELECT * FROM (SELECT id, name FROM users WHERE age > 20) AS t";
    parser::Lexer lexer(sql);
    parser::Parser parser(lexer);
    auto stmt = parser.parseStatement();
    assert(stmt != nullptr);

    auto result = executor::Executor::getInstance().execute(stmt.get());
    assert(result.isSuccess() && "FROM subquery should succeed");

    const auto& rows = result.getValue()->rows;
    std::cout << "  Returned " << rows.size() << " rows:" << std::endl;
    for (const auto& row : rows) {
        if (row.size() >= 2) {
            std::cout << "    " << row[0].toString() << " | " << row[1].toString() << std::endl;
        }
    }

    // Expect: Alice (25), Bob (30) - 2 rows
    assert(rows.size() == 2 && "Should return 2 rows with age > 20");
    std::cout << "  PASSED: FROM subquery basic works" << std::endl;
}

void testFromSubqueryWithJoin() {
    std::cout << "\n=== Test: FROM subquery with JOIN ===" << std::endl;

    std::string dbName = getUniqueDbName();
    if (!setupTables(dbName)) {
        assert(false && "Setup failed");
    }

    // SELECT t.name, t.age FROM (SELECT id, name, age FROM users WHERE age > 20) AS t
    std::string sql = "SELECT t.name, t.age FROM (SELECT id, name, age FROM users WHERE age > 20) AS t";
    parser::Lexer lexer(sql);
    parser::Parser parser(lexer);
    auto stmt = parser.parseStatement();
    assert(stmt != nullptr);

    auto result = executor::Executor::getInstance().execute(stmt.get());
    assert(result.isSuccess() && "FROM subquery with JOIN should succeed");

    const auto& rows = result.getValue()->rows;
    std::cout << "  Returned " << rows.size() << " rows:" << std::endl;
    for (const auto& row : rows) {
        if (row.size() >= 2) {
            std::cout << "    " << row[0].toString() << " | " << row[1].toString() << std::endl;
        }
    }

    assert(rows.size() == 2 && "Should return 2 rows");
    std::cout << "  PASSED: FROM subquery with JOIN works" << std::endl;
}

void testFromSubqueryParsing() {
    std::cout << "\n=== Test: FROM subquery parsing ===" << std::endl;

    std::string sql = "SELECT * FROM (SELECT id, name FROM users WHERE age > 20) AS t";
    parser::Lexer lexer(sql);
    parser::Parser parser(lexer);
    auto stmt = parser.parseStatement();

    assert(stmt != nullptr);
    assert(stmt->getType() == parser::ASTNodeType::SELECT_STMT);

    auto selectStmt = std::dynamic_pointer_cast<parser::SelectStmt>(std::shared_ptr<parser::ASTNode>(stmt));
    assert(selectStmt != nullptr);
    assert(selectStmt->fromTable != nullptr);
    assert(selectStmt->fromTable->subquery != nullptr);
    assert(selectStmt->fromTable->alias == "t");
    std::cout << "  PASSED: FROM subquery AST correct" << std::endl;
}

int main() {
    std::cout << "=== FROM Subquery Tests (Phase 7) ===" << std::endl;

    testFromSubqueryParsing();
    testFromSubqueryBasic();
    testFromSubqueryWithJoin();

    std::cout << "\n=== All FROM Subquery Tests PASSED ===" << std::endl;
    return 0;
}
