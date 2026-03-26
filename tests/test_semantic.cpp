#include "../src/parser/SemanticAnalyzer.h"
#include "../src/parser/Lexer.h"
#include "../src/parser/Parser.h"
#include "../src/storage/TableManager.h"
#include "test_utils.h"
#include <iostream>

using namespace minisql;
using namespace parser;

// ============================================================
// 辅助函数
// ============================================================

std::shared_ptr<ASTNode> parseAndAnalyze(const std::string& sql) {
    Lexer lexer(sql);
    Parser parser(lexer);
    auto stmt = parser.parseStatement();
    SemanticAnalyzer analyzer;
    analyzer.analyze(stmt);
    return stmt;
}

std::shared_ptr<ASTNode> parseOnly(const std::string& sql) {
    Lexer lexer(sql);
    Parser parser(lexer);
    return parser.parseStatement();
}

// ============================================================
// SemanticAnalyzer 基础测试
// ============================================================

TEST(testSemanticAnalyzerBasic) {
    SemanticAnalyzer analyzer;
    ASSERT_TRUE(true);
}

// ============================================================
// USE 语句语义分析测试
// ============================================================

TEST(testSemanticAnalyzerUseDatabase) {
    storage::TableManager& tm = storage::TableManager::getInstance();
    tm.createDatabase("test_use_db");

    Lexer lexer("USE test_use_db;");
    Parser parser(lexer);
    auto stmt = parser.parseStatement();

    SemanticAnalyzer analyzer;
    bool success = false;
    try {
        analyzer.analyze(stmt);
        success = true;
    } catch (const SemanticError& e) {
        success = false;
    }
    ASSERT_TRUE(success);

    tm.dropDatabase("test_use_db");
}

TEST(testSemanticAnalyzerUseNonexistentDatabase) {
    Lexer lexer("USE nonexistent_db;");
    Parser parser(lexer);
    auto stmt = parser.parseStatement();

    SemanticAnalyzer analyzer;
    bool threwError = false;
    try {
        analyzer.analyze(stmt);
    } catch (const SemanticError& e) {
        threwError = true;
    }
    ASSERT_TRUE(threwError);
}

// ============================================================
// SELECT 语句语义分析测试
// ============================================================

TEST(testSemanticAnalyzerSelectFromExistingTable) {
    storage::TableManager& tm = storage::TableManager::getInstance();
    tm.createDatabase("test_select_db");

    TableDef tableDef;
    tableDef.name = "users";
    tableDef.database = "test_select_db";

    ColumnDef idCol;
    idCol.name = "id";
    idCol.type = DataType::INT;
    tableDef.columns.push_back(idCol);

    ColumnDef nameCol;
    nameCol.name = "name";
    nameCol.type = DataType::VARCHAR;
    nameCol.length = 100;
    tableDef.columns.push_back(nameCol);

    tm.createTable("test_select_db", tableDef);

    // 分析 SELECT 语句（当前没有数据库上下文，语义检查会跳过）
    Lexer lexer("SELECT id, name FROM users;");
    Parser parser(lexer);
    auto stmt = parser.parseStatement();

    SemanticAnalyzer analyzer;
    bool success = false;
    try {
        analyzer.analyze(stmt);
        success = true;
    } catch (const SemanticError& e) {
        success = false;
    }
    // 因为没有设置当前数据库，所以不会抛出错误
    ASSERT_TRUE(success);

    tm.dropDatabase("test_select_db");
}

TEST(testSemanticAnalyzerSelectWithWhere) {
    storage::TableManager& tm = storage::TableManager::getInstance();
    tm.createDatabase("test_where_db");

    TableDef tableDef;
    tableDef.name = "products";
    tableDef.database = "test_where_db";

    ColumnDef idCol;
    idCol.name = "id";
    idCol.type = DataType::INT;
    tableDef.columns.push_back(idCol);

    ColumnDef priceCol;
    priceCol.name = "price";
    priceCol.type = DataType::DOUBLE;
    tableDef.columns.push_back(priceCol);

    tm.createTable("test_where_db", tableDef);

    Lexer lexer("SELECT * FROM products WHERE price > 100.0;");
    Parser parser(lexer);
    auto stmt = parser.parseStatement();

    SemanticAnalyzer analyzer;
    bool success = false;
    try {
        analyzer.analyze(stmt);
        success = true;
    } catch (const SemanticError& e) {
        success = false;
    }
    ASSERT_TRUE(success);

    tm.dropDatabase("test_where_db");
}

// ============================================================
// INSERT 语句语义分析测试
// ============================================================

TEST(testSemanticAnalyzerInsertIntoExistingTable) {
    storage::TableManager& tm = storage::TableManager::getInstance();
    tm.createDatabase("test_insert_db");

    TableDef tableDef;
    tableDef.name = "items";
    tableDef.database = "test_insert_db";

    ColumnDef idCol;
    idCol.name = "id";
    idCol.type = DataType::INT;
    tableDef.columns.push_back(idCol);

    ColumnDef descCol;
    descCol.name = "description";
    descCol.type = DataType::VARCHAR;
    descCol.length = 200;
    tableDef.columns.push_back(descCol);

    tm.createTable("test_insert_db", tableDef);

    Lexer lexer("INSERT INTO items (id, description) VALUES (1, 'test item');");
    Parser parser(lexer);
    auto stmt = parser.parseStatement();

    SemanticAnalyzer analyzer;
    bool success = false;
    try {
        analyzer.analyze(stmt);
        success = true;
    } catch (const SemanticError& e) {
        success = false;
    }
    ASSERT_TRUE(success);

    tm.dropDatabase("test_insert_db");
}

// ============================================================
// UPDATE 语句语义分析测试
// ============================================================

TEST(testSemanticAnalyzerUpdateExistingTable) {
    storage::TableManager& tm = storage::TableManager::getInstance();
    tm.createDatabase("test_update_db");

    TableDef tableDef;
    tableDef.name = "users";
    tableDef.database = "test_update_db";

    ColumnDef idCol;
    idCol.name = "id";
    idCol.type = DataType::INT;
    tableDef.columns.push_back(idCol);

    ColumnDef ageCol;
    ageCol.name = "age";
    ageCol.type = DataType::INT;
    tableDef.columns.push_back(ageCol);

    tm.createTable("test_update_db", tableDef);

    Lexer lexer("UPDATE users SET age = 25 WHERE id = 1;");
    Parser parser(lexer);
    auto stmt = parser.parseStatement();

    SemanticAnalyzer analyzer;
    bool success = false;
    try {
        analyzer.analyze(stmt);
        success = true;
    } catch (const SemanticError& e) {
        success = false;
    }
    ASSERT_TRUE(success);

    tm.dropDatabase("test_update_db");
}

// ============================================================
// DELETE 语句语义分析测试
// ============================================================

TEST(testSemanticAnalyzerDeleteFromExistingTable) {
    storage::TableManager& tm = storage::TableManager::getInstance();
    tm.createDatabase("test_delete_db");

    TableDef tableDef;
    tableDef.name = "orders";
    tableDef.database = "test_delete_db";

    ColumnDef idCol;
    idCol.name = "id";
    idCol.type = DataType::INT;
    tableDef.columns.push_back(idCol);

    tm.createTable("test_delete_db", tableDef);

    Lexer lexer("DELETE FROM orders WHERE id = 1;");
    Parser parser(lexer);
    auto stmt = parser.parseStatement();

    SemanticAnalyzer analyzer;
    bool success = false;
    try {
        analyzer.analyze(stmt);
        success = true;
    } catch (const SemanticError& e) {
        success = false;
    }
    ASSERT_TRUE(success);

    tm.dropDatabase("test_delete_db");
}

// ============================================================
// CREATE TABLE 语句语义分析测试
// ============================================================

TEST(testSemanticAnalyzerCreateTableWithValidColumns) {
    storage::TableManager& tm = storage::TableManager::getInstance();
    tm.createDatabase("test_create_db2");

    // 确保 test_create_db2 数据库存在
    bool dbExists = tm.databaseExists("test_create_db2");
    ASSERT_TRUE(dbExists);

    // 测试用 IF NOT EXISTS 创建已存在的表（不会报错）
    // 先创建一个表
    TableDef tableDef;
    tableDef.name = "existing_table";
    tableDef.database = "test_create_db2";

    ColumnDef idCol;
    idCol.name = "id";
    idCol.type = DataType::INT;
    tableDef.columns.push_back(idCol);

    auto result = tm.createTable("test_create_db2", tableDef);
    ASSERT_TRUE(result.isSuccess());

    // 使用 IF NOT EXISTS 解析已存在的表（应该成功）
    Lexer lexer("CREATE TABLE IF NOT EXISTS existing_table (id INT);");
    Parser parser(lexer);
    auto stmt = parser.parseStatement();

    SemanticAnalyzer analyzer;
    bool success = false;
    try {
        analyzer.analyze(stmt);
        success = true;
    } catch (const SemanticError& e) {
        success = false;
    }
    ASSERT_TRUE(success);

    tm.dropDatabase("test_create_db2");
}

// ============================================================
// DROP 语句语义分析测试
// ============================================================

TEST(testSemanticAnalyzerDropTable) {
    storage::TableManager& tm = storage::TableManager::getInstance();
    tm.createDatabase("test_drop_db");

    TableDef tableDef;
    tableDef.name = "temp_table";
    tableDef.database = "test_drop_db";

    ColumnDef idCol;
    idCol.name = "id";
    idCol.type = DataType::INT;
    tableDef.columns.push_back(idCol);

    tm.createTable("test_drop_db", tableDef);

    Lexer lexer("DROP TABLE temp_table;");
    Parser parser(lexer);
    auto stmt = parser.parseStatement();

    SemanticAnalyzer analyzer;
    bool success = false;
    try {
        analyzer.analyze(stmt);
        success = true;
    } catch (const SemanticError& e) {
        success = false;
    }
    ASSERT_TRUE(success);

    tm.dropDatabase("test_drop_db");
}

// ============================================================
// SHOW 语句语义分析测试
// ============================================================

TEST(testSemanticAnalyzerShowDatabases) {
    Lexer lexer("SHOW DATABASES;");
    Parser parser(lexer);
    auto stmt = parser.parseStatement();

    SemanticAnalyzer analyzer;
    bool success = false;
    try {
        analyzer.analyze(stmt);
        success = true;
    } catch (const SemanticError& e) {
        success = false;
    }
    ASSERT_TRUE(success);
}

// ============================================================
// 集成测试 - 完整语句流程
// ============================================================

TEST(testSemanticAnalyzerFullWorkflow) {
    storage::TableManager& tm = storage::TableManager::getInstance();
    tm.createDatabase("test_workflow_db");

    // 1. 创建表
    TableDef tableDef;
    tableDef.name = "employees";
    tableDef.database = "test_workflow_db";

    ColumnDef idCol;
    idCol.name = "id";
    idCol.type = DataType::INT;
    idCol.primaryKey = true;
    tableDef.columns.push_back(idCol);

    ColumnDef nameCol;
    nameCol.name = "name";
    nameCol.type = DataType::VARCHAR;
    nameCol.length = 100;
    nameCol.notNull = true;
    tableDef.columns.push_back(nameCol);

    ColumnDef salaryCol;
    salaryCol.name = "salary";
    salaryCol.type = DataType::DOUBLE;
    tableDef.columns.push_back(salaryCol);

    tm.createTable("test_workflow_db", tableDef);

    // 2. 解析并分析 INSERT
    {
        Lexer lexer1("INSERT INTO employees (id, name, salary) VALUES (1, 'John', 5000.0);");
        Parser parser1(lexer1);
        auto stmt1 = parser1.parseStatement();
        SemanticAnalyzer analyzer1;
        bool success = false;
        try {
            analyzer1.analyze(stmt1);
            success = true;
        } catch (const SemanticError& e) {
            success = false;
        }
        ASSERT_TRUE(success);
    }

    // 3. 解析并分析 SELECT
    {
        Lexer lexer2("SELECT id, name, salary FROM employees WHERE salary > 3000;");
        Parser parser2(lexer2);
        auto stmt2 = parser2.parseStatement();
        SemanticAnalyzer analyzer2;
        bool success = false;
        try {
            analyzer2.analyze(stmt2);
            success = true;
        } catch (const SemanticError& e) {
            success = false;
        }
        ASSERT_TRUE(success);
    }

    // 4. 解析并分析 UPDATE
    {
        Lexer lexer3("UPDATE employees SET salary = 5500.0 WHERE id = 1;");
        Parser parser3(lexer3);
        auto stmt3 = parser3.parseStatement();
        SemanticAnalyzer analyzer3;
        bool success = false;
        try {
            analyzer3.analyze(stmt3);
            success = true;
        } catch (const SemanticError& e) {
            success = false;
        }
        ASSERT_TRUE(success);
    }

    // 清理
    tm.dropDatabase("test_workflow_db");
}

// ============================================================
// 主函数
// ============================================================

int main() {
    std::cout << "=== SemanticAnalyzer Tests ===" << std::endl;

    // 基础测试
    testSemanticAnalyzerBasic();

    // USE 语句测试
    testSemanticAnalyzerUseDatabase();
    testSemanticAnalyzerUseNonexistentDatabase();

    // SELECT 语句测试
    testSemanticAnalyzerSelectFromExistingTable();
    testSemanticAnalyzerSelectWithWhere();

    // INSERT 语句测试
    testSemanticAnalyzerInsertIntoExistingTable();

    // UPDATE 语句测试
    testSemanticAnalyzerUpdateExistingTable();

    // DELETE 语句测试
    testSemanticAnalyzerDeleteFromExistingTable();

    // CREATE TABLE 语句测试
    testSemanticAnalyzerCreateTableWithValidColumns();

    // DROP 语句测试
    testSemanticAnalyzerDropTable();

    // SHOW 语句测试
    testSemanticAnalyzerShowDatabases();

    // 集成测试
    testSemanticAnalyzerFullWorkflow();

    RUN_TESTS;

    return 0;
}
