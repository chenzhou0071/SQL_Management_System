#include "../src/executor/DDLExecutor.h"
#include "../src/storage/TableManager.h"
#include "../src/common/Type.h"
#include <cassert>
#include <iostream>

using namespace minisql;

const std::string DB_NAME = "test_ddl_db";

void cleanup() {
    auto& tableMgr = storage::TableManager::getInstance();
    if (tableMgr.databaseExists(DB_NAME)) {
        tableMgr.dropDatabase(DB_NAME);
    }
}

void testCreateDatabase() {
    std::cout << "Running testCreateDatabase..." << std::endl;

    cleanup();

    // 使用新的 AST 节点
    parser::CreateDatabaseStmt stmt;
    stmt.database = DB_NAME;

    auto result = executor::DDLExecutor::executeCreateDatabase(&stmt);
    assert(result.isSuccess());

    // 验证数据库已创建
    auto& tableMgr = storage::TableManager::getInstance();
    assert(tableMgr.databaseExists(DB_NAME));

    std::cout << "testCreateDatabase: PASSED" << std::endl;
}

void testCreateTable() {
    std::cout << "Running testCreateTable..." << std::endl;

    // 创建数据库
    cleanup();
    parser::CreateDatabaseStmt createDbStmt;
    createDbStmt.database = DB_NAME;
    executor::DDLExecutor::executeCreateDatabase(&createDbStmt);

    // 创建表定义
    parser::CreateTableStmt stmt;
    stmt.table = "users";

    auto col1 = std::make_shared<parser::ColumnDefNode>();
    col1->name = "id";
    col1->type = DataType::INT;
    col1->primaryKey = true;
    stmt.columns.push_back(col1);

    auto col2 = std::make_shared<parser::ColumnDefNode>();
    col2->name = "name";
    col2->type = DataType::VARCHAR;
    col2->length = 50;
    stmt.columns.push_back(col2);

    auto result = executor::DDLExecutor::executeCreateTable(DB_NAME, &stmt);
    assert(result.isSuccess());

    // 验证表已创建
    auto& tableMgr = storage::TableManager::getInstance();
    auto tables = tableMgr.listTables(DB_NAME);
    assert(tables.isSuccess());
    assert(tables.getValue()->size() == 1);
    assert(tables.getValue()->at(0) == "users");

    std::cout << "testCreateTable: PASSED" << std::endl;
}

void testDropTable() {
    std::cout << "Running testDropTable..." << std::endl;

    // 准备：创建数据库和表
    cleanup();
    parser::CreateDatabaseStmt createDbStmt;
    createDbStmt.database = DB_NAME;
    executor::DDLExecutor::executeCreateDatabase(&createDbStmt);

    parser::CreateTableStmt createStmt;
    createStmt.table = "temp_table";
    auto col = std::make_shared<parser::ColumnDefNode>();
    col->name = "id";
    col->type = DataType::INT;
    createStmt.columns.push_back(col);
    executor::DDLExecutor::executeCreateTable(DB_NAME, &createStmt);

    // 删除表
    parser::DropStmt dropStmt;
    dropStmt.objectType = "TABLE";
    dropStmt.name = "temp_table";

    auto result = executor::DDLExecutor::executeDrop(DB_NAME, &dropStmt);
    assert(result.isSuccess());

    // 验证表已删除
    auto& tableMgr = storage::TableManager::getInstance();
    auto tables = tableMgr.listTables(DB_NAME);
    assert(tables.isSuccess());
    assert(tables.getValue()->empty());

    std::cout << "testDropTable: PASSED" << std::endl;
}

void testDropDatabase() {
    std::cout << "Running testDropDatabase..." << std::endl;

    // 准备：创建数据库
    cleanup();
    parser::CreateDatabaseStmt createDbStmt;
    createDbStmt.database = DB_NAME;
    executor::DDLExecutor::executeCreateDatabase(&createDbStmt);

    // 删除数据库
    parser::DropStmt dropStmt;
    dropStmt.objectType = "DATABASE";
    dropStmt.name = DB_NAME;

    auto result = executor::DDLExecutor::executeDrop("", &dropStmt);
    assert(result.isSuccess());

    // 验证数据库已删除
    auto& tableMgr = storage::TableManager::getInstance();
    assert(!tableMgr.databaseExists(DB_NAME));

    std::cout << "testDropDatabase: PASSED" << std::endl;
}

void testUseDatabase() {
    std::cout << "Running testUseDatabase..." << std::endl;

    // 准备：创建数据库
    cleanup();
    parser::CreateDatabaseStmt createDbStmt;
    createDbStmt.database = DB_NAME;
    executor::DDLExecutor::executeCreateDatabase(&createDbStmt);

    // 使用数据库
    parser::UseStmt useStmt;
    useStmt.database = DB_NAME;

    auto result = executor::DDLExecutor::executeUseDatabase(&useStmt);
    assert(result.isSuccess());

    // 验证返回的数据库名
    assert(result.getValue()->rows.size() == 1);
    assert(result.getValue()->rows[0][0].getString() == DB_NAME);

    std::cout << "testUseDatabase: PASSED" << std::endl;
}

void testCreateIndex() {
    std::cout << "Running testCreateIndex..." << std::endl;

    // 准备：创建数据库和表
    cleanup();
    parser::CreateDatabaseStmt createDbStmt;
    createDbStmt.database = DB_NAME;
    executor::DDLExecutor::executeCreateDatabase(&createDbStmt);

    parser::CreateTableStmt createTableStmt;
    createTableStmt.table = "users";
    auto col = std::make_shared<parser::ColumnDefNode>();
    col->name = "id";
    col->type = DataType::INT;
    createTableStmt.columns.push_back(col);
    executor::DDLExecutor::executeCreateTable(DB_NAME, &createTableStmt);

    // 创建索引
    parser::CreateIndexStmt createIdxStmt;
    createIdxStmt.indexName = "idx_id";
    createIdxStmt.tableName = "users";
    createIdxStmt.columnNames.push_back("id");
    createIdxStmt.unique = false;

    auto result = executor::DDLExecutor::executeCreateIndex(DB_NAME, &createIdxStmt);
    assert(result.isSuccess());

    std::cout << "testCreateIndex: PASSED" << std::endl;
}

void testDropIndex() {
    std::cout << "Running testDropIndex..." << std::endl;

    // 准备：创建数据库、表和索引
    cleanup();
    parser::CreateDatabaseStmt createDbStmt;
    createDbStmt.database = DB_NAME;
    executor::DDLExecutor::executeCreateDatabase(&createDbStmt);

    parser::CreateTableStmt createTableStmt;
    createTableStmt.table = "users";
    auto col = std::make_shared<parser::ColumnDefNode>();
    col->name = "id";
    col->type = DataType::INT;
    createTableStmt.columns.push_back(col);
    executor::DDLExecutor::executeCreateTable(DB_NAME, &createTableStmt);

    parser::CreateIndexStmt createIdxStmt;
    createIdxStmt.indexName = "idx_id";
    createIdxStmt.tableName = "users";
    createIdxStmt.columnNames.push_back("id");
    executor::DDLExecutor::executeCreateIndex(DB_NAME, &createIdxStmt);

    // 删除索引
    parser::DropStmt dropIdxStmt;
    dropIdxStmt.objectType = "INDEX";
    dropIdxStmt.name = "idx_id";

    auto result = executor::DDLExecutor::executeDrop(DB_NAME, &dropIdxStmt);
    assert(result.isSuccess());

    std::cout << "testDropIndex: PASSED" << std::endl;
}

int main() {
    std::cout << "=== DDLExecutor Tests ===" << std::endl;

    testCreateDatabase();
    testCreateTable();
    testDropTable();
    testDropDatabase();
    testUseDatabase();
    testCreateIndex();
    testDropIndex();

    std::cout << "\n=== All tests PASSED ===" << std::endl;
    return 0;
}
