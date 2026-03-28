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
    return "test_join_" + std::to_string(std::time(nullptr));
}

bool setupJoinTables(const std::string& dbName) {
    storage::TableManager::getInstance().dropDatabase(dbName);
    auto dbResult = storage::TableManager::getInstance().createDatabase(dbName);
    if (dbResult.isError()) return false;
    executor::Executor::getInstance().setCurrentDatabase(dbName);

    // Create users table
    {
        std::string sql = "CREATE TABLE users (id INT PRIMARY KEY, name VARCHAR(50))";
        parser::Lexer lexer(sql);
        parser::Parser parser(lexer);
        auto stmt = parser.parseStatement();
        if (!stmt) return false;
        auto r = executor::Executor::getInstance().execute(stmt.get());
        if (r.isError()) return false;
    }

    // Insert users: (1, 'Alice'), (2, 'Bob'), (3, 'Charlie')
    for (const auto& [id, name] : std::vector<std::pair<std::string, std::string>>{
             {"1", "'Alice'"}, {"2", "'Bob'"}, {"3", "'Charlie'"}}) {
        std::string sql = "INSERT INTO users (id, name) VALUES (" + id + ", " + name + ")";
        parser::Lexer lexer(sql);
        parser::Parser parser(lexer);
        auto stmt = parser.parseStatement();
        if (stmt) executor::Executor::getInstance().execute(stmt.get());
    }

    // Create orders table
    {
        std::string sql = "CREATE TABLE orders (id INT PRIMARY KEY, user_id INT, amount INT)";
        parser::Lexer lexer(sql);
        parser::Parser parser(lexer);
        auto stmt = parser.parseStatement();
        if (!stmt) return false;
        auto r = executor::Executor::getInstance().execute(stmt.get());
        if (r.isError()) return false;
    }

    // Insert orders: (1, 1, 100), (2, 1, 200), (3, 2, 150), (4, 5, 999) — 5 doesn't exist
    for (const auto& [id, uid, amt] : std::vector<std::tuple<std::string, std::string, std::string>>{
             {"1", "1", "100"}, {"2", "1", "200"}, {"3", "2", "150"}, {"4", "5", "999"}}) {
        std::string sql = "INSERT INTO orders (id, user_id, amount) VALUES (" + id + ", " + uid + ", " + amt + ")";
        parser::Lexer lexer(sql);
        parser::Parser parser(lexer);
        auto stmt = parser.parseStatement();
        if (stmt) executor::Executor::getInstance().execute(stmt.get());
    }

    return true;
}

// ============================================================
// TDD Step 1: INNER JOIN — 验证计划生成并返回结果
// ============================================================

void testInnerJoinBasic() {
    std::cout << "\n=== Test: INNER JOIN basic ===" << std::endl;

    std::string dbName = getUniqueDbName();
    if (!setupJoinTables(dbName)) {
        assert(false && "Setup failed");
    }

    // SELECT users.name, orders.amount FROM users JOIN orders ON users.id = orders.user_id
    std::string sql = "SELECT users.name, orders.amount FROM users JOIN orders ON users.id = orders.user_id";
    parser::Lexer lexer(sql);
    parser::Parser parser(lexer);
    auto stmt = parser.parseStatement();
    assert(stmt != nullptr);

    auto result = executor::Executor::getInstance().execute(stmt.get());
    assert(result.isSuccess() && "INNER JOIN should succeed");

    const auto& rows = result.getValue()->rows;
    std::cout << "  Returned " << rows.size() << " rows" << std::endl;

    // Expect: Alice-100, Alice-200, Bob-150 (3 rows)
    assert(rows.size() == 3 && "Should return 3 matching rows");
    std::cout << "  PASSED: INNER JOIN returned 3 rows" << std::endl;
}

// ============================================================
// TDD Step 2: LEFT JOIN — 验证计划生成， LEFT 带 NULL 行
// ============================================================

void testLeftJoin() {
    std::cout << "\n=== Test: LEFT JOIN ===" << std::endl;

    std::string dbName = getUniqueDbName();
    if (!setupJoinTables(dbName)) {
        assert(false && "Setup failed");
    }

    // Charlie (id=3) has no orders — should appear with NULL
    std::string sql = "SELECT users.name, orders.amount FROM users LEFT JOIN orders ON users.id = orders.user_id";
    parser::Lexer lexer(sql);
    parser::Parser parser(lexer);
    auto stmt = parser.parseStatement();
    assert(stmt != nullptr);

    auto result = executor::Executor::getInstance().execute(stmt.get());
    assert(result.isSuccess() && "LEFT JOIN should succeed");

    const auto& rows = result.getValue()->rows;
    std::cout << "  Returned " << rows.size() << " rows:" << std::endl;
    for (const auto& row : rows) {
        if (row.size() >= 2) {
            std::cout << "    " << row[0].toString() << " | " << (row[1].isNull() ? "NULL" : row[1].toString()) << std::endl;
        }
    }

    // Expect: Alice-100, Alice-200, Bob-150, Charlie-NULL (4 rows)
    assert(rows.size() == 4 && "LEFT JOIN should return all left rows including NULL");

    // Find Charlie row
    bool foundCharlie = false;
    for (const auto& row : rows) {
        if (row.size() >= 1 && row[0].getString() == "Charlie") {
            foundCharlie = true;
            assert(row[1].isNull() && "Charlie's order amount should be NULL");
        }
    }
    assert(foundCharlie && "Charlie should be in result with NULL amount");
    std::cout << "  PASSED: LEFT JOIN returned Charlie with NULL" << std::endl;
}

// ============================================================
// TDD Step 3: Multi-table JOIN — 三个表的 JOIN
// ============================================================

void testMultiTableJoin() {
    std::cout << "\n=== Test: Multi-table JOIN (3 tables) ===" << std::endl;

    std::string dbName = getUniqueDbName();
    storage::TableManager::getInstance().dropDatabase(dbName);
    storage::TableManager::getInstance().createDatabase(dbName);
    executor::Executor::getInstance().setCurrentDatabase(dbName);

    // CREATE TABLE a (id INT PRIMARY KEY, name VARCHAR(50))
    for (const auto& sql : {
        "CREATE TABLE a (id INT PRIMARY KEY, name VARCHAR(50))",
        "CREATE TABLE b (id INT PRIMARY KEY, a_id INT, val INT)",
        "CREATE TABLE c (id INT PRIMARY KEY, b_id INT, desc VARCHAR(50))"
    }) {
        parser::Lexer lexer(sql);
        parser::Parser parser(lexer);
        auto stmt = parser.parseStatement();
        assert(stmt);
        auto r = executor::Executor::getInstance().execute(stmt.get());
        assert(r.isSuccess());
    }

    // Insert: a{1,'X'}, b{1,1,10}, c{1,1,'good'}
    for (const auto& sql : {
        "INSERT INTO a (id, name) VALUES (1, 'X')",
        "INSERT INTO b (id, a_id, val) VALUES (1, 1, 10)",
        "INSERT INTO c (id, b_id, desc) VALUES (1, 1, 'good')"
    }) {
        parser::Lexer lexer(sql);
        parser::Parser parser(lexer);
        auto stmt = parser.parseStatement();
        if (stmt) executor::Executor::getInstance().execute(stmt.get());
    }

    // SELECT a.name, c.desc FROM a JOIN b ON a.id = b.a_id JOIN c ON b.id = c.b_id
    std::string sql = "SELECT a.name, c.desc FROM a JOIN b ON a.id = b.a_id JOIN c ON b.id = c.b_id";
    parser::Lexer lexer(sql);
    parser::Parser parser(lexer);
    auto stmt = parser.parseStatement();
    assert(stmt != nullptr);

    auto result = executor::Executor::getInstance().execute(stmt.get());
    assert(result.isSuccess() && "Multi-table JOIN should succeed");

    const auto& rows = result.getValue()->rows;
    std::cout << "  Returned " << rows.size() << " rows" << std::endl;
    assert(rows.size() == 1 && "Should return 1 joined row");
    assert(rows[0].size() >= 2);
    assert(rows[0][0].getString() == "X");
    assert(rows[0][1].getString() == "good");
    std::cout << "  PASSED: Multi-table JOIN works" << std::endl;
}

// ============================================================
// TDD Step 4: JOIN with WHERE clause
// ============================================================


	void testJoinWithWhere() {
	    std::cout << "\n=== Test: JOIN with WHERE ===" << std::endl;

	    std::string dbName = getUniqueDbName();
	    if (!setupJoinTables(dbName)) {
	        assert(false && "Setup failed");
	    }

	    // SELECT users.name, orders.amount FROM users JOIN orders ON users.id = orders.user_id WHERE orders.amount > 100
	    std::string sql = "SELECT users.name, orders.amount FROM users JOIN orders ON users.id = orders.user_id WHERE orders.amount > 100";
	    parser::Lexer lexer(sql);
	    parser::Parser parser(lexer);
	    auto stmt = parser.parseStatement();
	    assert(stmt != nullptr);

	    auto result = executor::Executor::getInstance().execute(stmt.get());
	    assert(result.isSuccess() && "JOIN with WHERE should succeed");

	    const auto& rows = result.getValue()->rows;
	    std::cout << "  Returned " << rows.size() << " rows:" << std::endl;
	    for (const auto& row : rows) {
	        if (row.size() >= 2) {
	            std::cout << "    " << row[0].toString() << " | " << row[1].toString() << std::endl;
	        }
	    }


		    // Expect: Alice-200, Bob-150 (amount > 100)
		    assert(rows.size() == 2);
		    assert(rows[0][0].getString() == "Alice" && rows[0][1].getInt() == 200);
		    assert(rows[1][0].getString() == "Bob" && rows[1][1].getInt() == 150);
	    std::cout << "  PASSED: JOIN with WHERE works correctly" << std::endl;
	}

// ============================================================
// TDD Step 5: RIGHT JOIN
// ============================================================

	void testRightJoin() {
	    std::cout << "\n=== Test: RIGHT JOIN ===" << std::endl;

	    std::string dbName = getUniqueDbName();
	    if (!setupJoinTables(dbName)) {
	        assert(false && "Setup failed");
	    }

	    // SELECT users.name, orders.amount FROM users RIGHT JOIN orders ON users.id = orders.user_id
	    // Orders: (1,1,100), (2,1,200), (3,2,150), (4,5,999)
	    // Users: (1,'Alice'), (2,'Bob'), (3,'Charlie')
	    // Expect: Alice-100, Alice-200, Bob-150, NULL-999 (order 4 doesn't match any user)
	    std::string sql = "SELECT users.name, orders.amount FROM users RIGHT JOIN orders ON users.id = orders.user_id";
	    parser::Lexer lexer(sql);
	    parser::Parser parser(lexer);
	    auto stmt = parser.parseStatement();
	    assert(stmt != nullptr);

	    auto result = executor::Executor::getInstance().execute(stmt.get());
	    assert(result.isSuccess() && "RIGHT JOIN should succeed");

	    const auto& rows = result.getValue()->rows;
	    std::cout << "  Returned " << rows.size() << " rows:" << std::endl;
	    for (const auto& row : rows) {
	        if (row.size() >= 2) {
	            std::cout << "    " << (row[0].isNull() ? "NULL" : row[0].toString()) << " | " << row[1].toString() << std::endl;
	        }
	    }

	    // Expect: 4 rows (Alice-100, Alice-200, Bob-150, NULL-999)
	    assert(rows.size() == 4 && "RIGHT JOIN should return all right rows including NULL");
	    
	    // Find order with id=4 (amount=999) that has no matching user
	    bool foundNullRow = false;
	    for (const auto& row : rows) {
	        if (row.size() >= 2 && row[1].getInt() == 999) {
	            foundNullRow = true;
	            assert(row[0].isNull() && "Order 999 should have NULL user name");
	        }
	    }
	    assert(foundNullRow && "Should find order with NULL user");
	    std::cout << "  PASSED: RIGHT JOIN works correctly" << std::endl;
	}

// ============================================================
// TDD: Verify JOIN parse creates JoinClause nodes
// ============================================================

void testJoinParsing() {
    std::cout << "\n=== Test: JOIN parsing creates AST nodes ===" << std::endl;

    std::string sql = "SELECT * FROM users JOIN orders ON users.id = orders.user_id";
    parser::Lexer lexer(sql);
    parser::Parser parser(lexer);
    auto stmt = parser.parseStatement();

    assert(stmt != nullptr);
    assert(stmt->getType() == parser::ASTNodeType::SELECT_STMT);

    auto selectStmt = std::dynamic_pointer_cast<parser::SelectStmt>(std::shared_ptr<parser::ASTNode>(stmt));
    assert(selectStmt != nullptr);
    assert(selectStmt->fromTable != nullptr);
    assert(selectStmt->fromTable->name == "users");

    // Check JOIN clause
    std::cout << "  Joins count: " << selectStmt->joins.size() << std::endl;
    assert(selectStmt->joins.size() == 1 && "Should have 1 JOIN clause");
    assert(selectStmt->joins[0]->joinType == "INNER" && "JOIN defaults to INNER");
    assert(selectStmt->joins[0]->table != nullptr);
    assert(selectStmt->joins[0]->table->name == "orders");
    assert(selectStmt->joins[0]->onCondition != nullptr && "ON condition should be set");
    std::cout << "  ON condition: " << selectStmt->joins[0]->onCondition->toString() << std::endl;
    std::cout << "  PASSED: JOIN AST nodes correct" << std::endl;
}

void testLeftJoinParsing() {
    std::cout << "\n=== Test: LEFT JOIN parsing ===" << std::endl;

    std::string sql = "SELECT * FROM users LEFT JOIN orders ON users.id = orders.user_id";
    parser::Lexer lexer(sql);
    parser::Parser parser(lexer);
    auto stmt = parser.parseStatement();

    assert(stmt != nullptr);
    auto selectStmt = std::dynamic_pointer_cast<parser::SelectStmt>(std::shared_ptr<parser::ASTNode>(stmt));
    assert(selectStmt->joins.size() == 1);
    assert(selectStmt->joins[0]->joinType == "LEFT");
    std::cout << "  PASSED: LEFT JOIN AST correct" << std::endl;
}

// ============================================================
// Main
// ============================================================

int main() {
    std::cout << "=== JOIN Executor Tests (Phase 7) ===" << std::endl;

    // Phase 1: Parsing tests (no execution needed)
    testJoinParsing();
    testLeftJoinParsing();

    // Phase 2: Execution tests
    testInnerJoinBasic();
    testLeftJoin();
    testMultiTableJoin();
    testJoinWithWhere();  // Skipped - column prefix bug
    testRightJoin();  // Skipped - depends on LEFT JOIN fix

    std::cout << "\n=== All JOIN Tests PASSED ===" << std::endl;
    return 0;
}
