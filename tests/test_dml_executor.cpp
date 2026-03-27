#include "../src/executor/DMLExecutor.h"
#include "../src/storage/TableManager.h"
#include "../src/parser/Parser.h"
#include "../src/common/Type.h"
#include <cassert>
#include <iostream>

using namespace minisql;

const std::string DB_NAME = "test_dml_db";

void prepareTestTable() {
    auto& tableMgr = storage::TableManager::getInstance();

    if (!tableMgr.databaseExists(DB_NAME)) {
        tableMgr.createDatabase(DB_NAME);
    }

    TableDef tableDef;
    tableDef.name = "users";
    tableDef.database = DB_NAME;

    // id 不是自增，以便测试直接插入
    ColumnDef col1;
    col1.name = "id";
    col1.type = DataType::INT;
    col1.primaryKey = true;
    col1.autoIncrement = false;

    ColumnDef col2;
    col2.name = "name";
    col2.type = DataType::VARCHAR;
    col2.length = 50;

    ColumnDef col3;
    col3.name = "age";
    col3.type = DataType::INT;

    tableDef.columns.push_back(col1);
    tableDef.columns.push_back(col2);
    tableDef.columns.push_back(col3);

    if (tableMgr.tableExists(DB_NAME, "users").getValue()) {
        tableMgr.dropTable(DB_NAME, "users");
    }
    tableMgr.createTable(DB_NAME, tableDef);
}

void testInsert() {
    std::cout << "Running testInsert..." << std::endl;

    // 创建 INSERT 语句: INSERT INTO users (id, name, age) VALUES (1, 'Alice', 25), (2, 'Bob', 30)
    parser::InsertStmt stmt;
    stmt.table = "users";

    stmt.columns = {"id", "name", "age"};

    // Alice, 25
    parser::ExprPtr aliceId = std::make_shared<parser::LiteralExpr>(Value(1));
    parser::ExprPtr aliceName = std::make_shared<parser::LiteralExpr>(Value("Alice"));
    parser::ExprPtr aliceAge = std::make_shared<parser::LiteralExpr>(Value(25));
    stmt.valuesList.push_back({aliceId, aliceName, aliceAge});

    // Bob, 30
    parser::ExprPtr bobId = std::make_shared<parser::LiteralExpr>(Value(2));
    parser::ExprPtr bobName = std::make_shared<parser::LiteralExpr>(Value("Bob"));
    parser::ExprPtr bobAge = std::make_shared<parser::LiteralExpr>(Value(30));
    stmt.valuesList.push_back({bobId, bobName, bobAge});

    auto result = executor::DMLExecutor::executeInsert(DB_NAME, &stmt);
    if (!result.isSuccess()) {
        std::cerr << "  ERROR: " << result.getError().getMessage() << std::endl;
    }
    assert(result.isSuccess());

    // 验证插入的行数
    assert(result.getValue()->rows.size() == 1);
    assert(result.getValue()->rows[0][0].getInt() == 2); // affected_rows = 2

    std::cout << "  Inserted 2 rows" << std::endl;
    std::cout << "testInsert: PASSED" << std::endl;
}

void testUpdate() {
    std::cout << "Running testUpdate..." << std::endl;

    // 创建 UPDATE 语句: UPDATE users SET age = 26 WHERE name = 'Alice'
    parser::UpdateStmt stmt;
    stmt.table = "users";

    // SET age = 26
    parser::ExprPtr ageValue = std::make_shared<parser::LiteralExpr>(Value(26));
    stmt.assignments.push_back({"age", ageValue});

    // WHERE name = 'Alice'
    parser::ExprPtr nameCol = std::make_shared<parser::ColumnRef>("", "name");
    parser::ExprPtr aliceValue = std::make_shared<parser::LiteralExpr>(Value("Alice"));
    parser::ExprPtr whereClause = std::make_shared<parser::BinaryExpr>(nameCol, "=", aliceValue);
    stmt.whereClause = whereClause;

    auto result = executor::DMLExecutor::executeUpdate(DB_NAME, &stmt);
    assert(result.isSuccess());

    // 验证更新的行数
    assert(result.getValue()->rows.size() == 1);
    assert(result.getValue()->rows[0][0].getInt() == 1); // affected_rows = 1

    std::cout << "  Updated 1 row" << std::endl;
    std::cout << "testUpdate: PASSED" << std::endl;
}

void testDelete() {
    std::cout << "Running testDelete..." << std::endl;

    // 创建 DELETE 语句: DELETE FROM users WHERE name = 'Bob'
    parser::DeleteStmt stmt;
    stmt.table = "users";

    // WHERE name = 'Bob'
    parser::ExprPtr nameCol = std::make_shared<parser::ColumnRef>("", "name");
    parser::ExprPtr bobValue = std::make_shared<parser::LiteralExpr>(Value("Bob"));
    parser::ExprPtr whereClause = std::make_shared<parser::BinaryExpr>(nameCol, "=", bobValue);
    stmt.whereClause = whereClause;

    auto result = executor::DMLExecutor::executeDelete(DB_NAME, &stmt);
    assert(result.isSuccess());

    // 验证删除的行数
    assert(result.getValue()->rows.size() == 1);
    assert(result.getValue()->rows[0][0].getInt() == 1); // affected_rows = 1

    std::cout << "  Deleted 1 row" << std::endl;
    std::cout << "testDelete: PASSED" << std::endl;
}

int main() {
    std::cout << "=== DMLExecutor Tests ===" << std::endl;

    prepareTestTable();
    testInsert();
    testUpdate();
    testDelete();

    std::cout << "\n=== All tests PASSED ===" << std::endl;
    return 0;
}
