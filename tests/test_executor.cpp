#include "../src/executor/Executor.h"
#include "../src/storage/TableManager.h"
#include "../src/parser/Parser.h"
#include "../src/parser/Lexer.h"
#include "../src/common/Type.h"
#include <cassert>
#include <iostream>

using namespace minisql;

const std::string DB_NAME = "test_executor_db";

void cleanup() {
    auto& tableMgr = storage::TableManager::getInstance();
    if (tableMgr.databaseExists(DB_NAME)) {
        tableMgr.dropDatabase(DB_NAME);
    }
    tableMgr.createDatabase(DB_NAME);
}

void prepareTestData() {
    auto& tableMgr = storage::TableManager::getInstance();

    TableDef tableDef;
    tableDef.name = "users";
    tableDef.database = DB_NAME;

    ColumnDef col1;
    col1.name = "id";
    col1.type = DataType::INT;
    col1.primaryKey = true;
    tableDef.columns.push_back(col1);

    ColumnDef col2;
    col2.name = "name";
    col2.type = DataType::VARCHAR;
    col2.length = 50;
    tableDef.columns.push_back(col2);

    ColumnDef col3;
    col3.name = "age";
    col3.type = DataType::INT;
    tableDef.columns.push_back(col3);

    tableMgr.createTable(DB_NAME, tableDef);

    Row row1 = {Value(1), Value("Alice"), Value(25)};
    Row row2 = {Value(2), Value("Bob"), Value(30)};
    Row row3 = {Value(3), Value("Charlie"), Value(25)};

    tableMgr.insertRow(DB_NAME, "users", row1);
    tableMgr.insertRow(DB_NAME, "users", row2);
    tableMgr.insertRow(DB_NAME, "users", row3);
}

void testSimpleSelect() {
    std::cout << "Running testSimpleSelect..." << std::endl;

    executor::Executor& executor = executor::Executor::getInstance();
    executor.setCurrentDatabase(DB_NAME);

    parser::Lexer lexer("SELECT * FROM users");
    parser::Parser parser(lexer);
    auto stmt = parser.parseStatement();
    assert(stmt != nullptr);

    auto execResult = executor.execute(stmt.get());
    assert(execResult.isSuccess());
    assert(execResult.getValue()->rows.size() == 3);

    std::cout << "  Got " << execResult.getValue()->rows.size() << " rows" << std::endl;
    std::cout << "testSimpleSelect: PASSED" << std::endl;
}

void testSelectWithWhere() {
    std::cout << "Running testSelectWithWhere..." << std::endl;

    executor::Executor& executor = executor::Executor::getInstance();
    executor.setCurrentDatabase(DB_NAME);

    parser::Lexer lexer("SELECT * FROM users WHERE age = 25");
    parser::Parser parser(lexer);
    auto stmt = parser.parseStatement();
    assert(stmt != nullptr);

    auto execResult = executor.execute(stmt.get());
    assert(execResult.isSuccess());
    assert(execResult.getValue()->rows.size() == 2);

    std::cout << "  Got " << execResult.getValue()->rows.size() << " rows matching WHERE" << std::endl;
    std::cout << "testSelectWithWhere: PASSED" << std::endl;
}

void testSelectWithLimit() {
    std::cout << "Running testSelectWithLimit..." << std::endl;

    executor::Executor& executor = executor::Executor::getInstance();
    executor.setCurrentDatabase(DB_NAME);

    parser::Lexer lexer("SELECT * FROM users LIMIT 2");
    parser::Parser parser(lexer);
    auto stmt = parser.parseStatement();
    assert(stmt != nullptr);

    auto execResult = executor.execute(stmt.get());
    assert(execResult.isSuccess());
    assert(execResult.getValue()->rows.size() == 2);

    std::cout << "  Got " << execResult.getValue()->rows.size() << " rows with LIMIT" << std::endl;
    std::cout << "testSelectWithLimit: PASSED" << std::endl;
}

void testInsert() {
    std::cout << "Running testInsert..." << std::endl;

    executor::Executor& executor = executor::Executor::getInstance();
    executor.setCurrentDatabase(DB_NAME);

    parser::Lexer lexer("INSERT INTO users VALUES (4, 'David', 35)");
    parser::Parser parser(lexer);
    auto stmt = parser.parseStatement();
    assert(stmt != nullptr);

    auto execResult = executor.execute(stmt.get());
    assert(execResult.isSuccess());

    // 验证
    parser::Lexer lexer2("SELECT * FROM users");
    parser::Parser parser2(lexer2);
    auto selectStmt = parser2.parseStatement();
    auto queryResult = executor.execute(selectStmt.get());
    assert(queryResult.getValue()->rows.size() == 4);

    std::cout << "  Inserted 1 row, total: " << queryResult.getValue()->rows.size() << std::endl;
    std::cout << "testInsert: PASSED" << std::endl;
}

int main() {
    std::cout << "=== Executor Tests ===" << std::endl;

    cleanup();
    prepareTestData();

    testSimpleSelect();
    testSelectWithWhere();
    testSelectWithLimit();
    testInsert();

    cleanup();

    std::cout << "\n=== All Executor tests PASSED ===" << std::endl;
    return 0;
}
