#include "../src/parser/Parser.h"
#include "../src/parser/Lexer.h"
#include "../src/parser/AST.h"
#include "../src/executor/Executor.h"
#include "../src/storage/TableManager.h"
#include "../src/common/Value.h"
#include <cassert>
#include <ctime>
#include <iostream>

using namespace minisql;

std::string getUniqueDbName() {
    return "test_having_" + std::to_string(std::time(nullptr));
}

bool setupTestTables(const std::string& dbName) {
    storage::TableManager::getInstance().dropDatabase(dbName);
    auto dbResult = storage::TableManager::getInstance().createDatabase(dbName);
    if (!dbResult.isSuccess()) return false;

    executor::Executor::getInstance().setCurrentDatabase(dbName);

    auto execSQL = [](const std::string& sql) {
        parser::Lexer lexer(sql);
        parser::Parser parser(lexer);
        auto stmt = parser.parseStatement();
        if (!stmt) return false;
        return executor::Executor::getInstance().execute(stmt.get()).isSuccess();
    };

    if (!execSQL("CREATE TABLE users (id INT, name VARCHAR(20), dept_id INT, salary INT)")) return false;
    if (!execSQL("CREATE TABLE orders (id INT, user_id INT, amount INT)")) return false;

    // Users: dept_id=1 有 3 人，dept_id=2 有 2 人
    execSQL("INSERT INTO users VALUES (1, 'Alice', 1, 5000)");
    execSQL("INSERT INTO users VALUES (2, 'Bob', 1, 6000)");
    execSQL("INSERT INTO users VALUES (3, 'Charlie', 2, 7000)");
    execSQL("INSERT INTO users VALUES (4, 'David', 2, 5500)");
    execSQL("INSERT INTO users VALUES (5, 'Eve', 1, 4500)");

    execSQL("INSERT INTO orders VALUES (1, 1, 100)");
    execSQL("INSERT INTO orders VALUES (2, 1, 200)");
    execSQL("INSERT INTO orders VALUES (3, 2, 150)");
    execSQL("INSERT INTO orders VALUES (4, 3, 80)");

    return true;
}

bool runTest(const std::string& name, const std::string& sql, int expectedRows) {
    std::cout << name << "\n";
    std::cout << "SQL: " << sql << "\n";

    parser::Lexer lexer(sql);
    parser::Parser parser(lexer);
    auto stmt = parser.parseStatement();
    if (!stmt) {
        std::cerr << "  FAIL: Parse error\n";
        return false;
    }

    auto result = executor::Executor::getInstance().execute(stmt.get());
    if (!result.isSuccess()) {
        std::cerr << "  FAIL: " << result.getError().getMessage() << "\n";
        return false;
    }

    const auto& rows = result.getValue()->rows;
    std::cout << "  Returned " << rows.size() << " rows (expected " << expectedRows << ")\n";
    for (const auto& row : rows) {
        std::cout << "    ";
        for (size_t i = 0; i < row.size(); ++i) {
            if (i > 0) std::cout << " | ";
            const Value& v = row[i];
            if (v.isNull()) std::cout << "NULL";
            else if (v.getType() == DataType::INT) std::cout << v.getInt();
            else if (v.getType() == DataType::DOUBLE) std::cout << v.getDouble();
            else if (v.getType() == DataType::VARCHAR) std::cout << v.getString();
        }
        std::cout << "\n";
    }

    bool pass = (rows.size() == static_cast<size_t>(expectedRows));
    std::cout << "  " << (pass ? "PASS" : "FAIL") << "\n\n";
    return pass;
}

int main() {
    std::cout << "=== HAVING 子句测试 (Phase 7) ===\n\n";

    std::string dbName = getUniqueDbName();
    if (!setupTestTables(dbName)) {
        std::cerr << "Failed to setup test tables\n";
        return 1;
    }

    int passed = 0, total = 0;

    // 测试1: 简单 SELECT（无 GROUP BY）
    if (runTest("测试1: 简单 SELECT",
        "SELECT id, name FROM users", 5)) {
        passed++;
    }
    total++;

    // 测试2: 简单 GROUP BY（无 HAVING）
    if (runTest("测试2: GROUP BY",
        "SELECT dept_id, COUNT(*) FROM users GROUP BY dept_id", 2)) {
        passed++;
    }
    total++;

    // 清理
    storage::TableManager::getInstance().dropDatabase(dbName);

    std::cout << "=======================================\n";
    std::cout << passed << "/" << total << " tests passed\n";
    return (passed == total) ? 0 : 1;
}
