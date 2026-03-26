#include "../src/parser/Parser.h"
#include "../src/parser/Lexer.h"
#include "../src/common/Error.h"
#include "test_utils.h"
#include <iostream>

using namespace minisql;
using namespace parser;

// ============================================================
// Parser 基础测试
// ============================================================

TEST(testParserEmptyInput) {
    Lexer lexer("");
    Parser parser(lexer);
    bool threwException = false;
    try {
        parser.parseStatement();
    } catch (const minisql::MiniSQLException& e) {
        threwException = true;
    }
    ASSERT_TRUE(threwException);
}

TEST(testParserParseUseStatement) {
    Lexer lexer("USE testdb;");
    Parser parser(lexer);
    auto stmt = parser.parseStatement();
    ASSERT_NOT_NULL(stmt.get());
    ASSERT_EQ(static_cast<int>(stmt->getType()), static_cast<int>(ASTNodeType::USE_STMT));
}

TEST(testParserParseShowDatabases) {
    Lexer lexer("SHOW DATABASES;");
    Parser parser(lexer);
    auto stmt = parser.parseStatement();
    ASSERT_NOT_NULL(stmt.get());
    ASSERT_EQ(static_cast<int>(stmt->getType()), static_cast<int>(ASTNodeType::SHOW_STMT));
}

TEST(testParserParseShowTables) {
    Lexer lexer("SHOW TABLES;");
    Parser parser(lexer);
    auto stmt = parser.parseStatement();
    ASSERT_NOT_NULL(stmt.get());
    ASSERT_EQ(static_cast<int>(stmt->getType()), static_cast<int>(ASTNodeType::SHOW_STMT));
}

TEST(testParserParseLiteralExpression) {
    Lexer lexer("SELECT 42;");
    Parser parser(lexer);
    auto stmt = parser.parseStatement();
    ASSERT_NOT_NULL(stmt.get());
    ASSERT_EQ(static_cast<int>(stmt->getType()), static_cast<int>(ASTNodeType::SELECT_STMT));
}

TEST(testParserParseColumnRef) {
    Lexer lexer("SELECT column1;");
    Parser parser(lexer);
    auto stmt = parser.parseStatement();
    ASSERT_NOT_NULL(stmt.get());
}

TEST(testParserParseBinaryExpression) {
    Lexer lexer("SELECT a + b;");
    Parser parser(lexer);
    auto stmt = parser.parseStatement();
    ASSERT_NOT_NULL(stmt.get());
}

TEST(testParserParseComparisonExpression) {
    Lexer lexer("SELECT * FROM t WHERE age > 18;");
    Parser parser(lexer);
    auto stmt = parser.parseStatement();
    ASSERT_NOT_NULL(stmt.get());
}

TEST(testParserParseLogicalExpression) {
    Lexer lexer("SELECT * FROM t WHERE a > 10 AND b < 20;");
    Parser parser(lexer);
    auto stmt = parser.parseStatement();
    ASSERT_NOT_NULL(stmt.get());
}

TEST(testParserParseInsertStatement) {
    Lexer lexer("INSERT INTO users (id, name) VALUES (1, 'Alice');");
    Parser parser(lexer);
    auto stmt = parser.parseStatement();
    ASSERT_NOT_NULL(stmt.get());
    ASSERT_EQ(static_cast<int>(stmt->getType()), static_cast<int>(ASTNodeType::INSERT_STMT));
}

TEST(testParserParseUpdateStatement) {
    Lexer lexer("UPDATE users SET age = 20 WHERE id = 1;");
    Parser parser(lexer);
    auto stmt = parser.parseStatement();
    ASSERT_NOT_NULL(stmt.get());
    ASSERT_EQ(static_cast<int>(stmt->getType()), static_cast<int>(ASTNodeType::UPDATE_STMT));
}

TEST(testParserParseDeleteStatement) {
    Lexer lexer("DELETE FROM users WHERE id = 1;");
    Parser parser(lexer);
    auto stmt = parser.parseStatement();
    ASSERT_NOT_NULL(stmt.get());
    ASSERT_EQ(static_cast<int>(stmt->getType()), static_cast<int>(ASTNodeType::DELETE_STMT));
}

TEST(testParserParseCreateTable) {
    Lexer lexer("CREATE TABLE users (id INT, name VARCHAR(100));");
    Parser parser(lexer);
    auto stmt = parser.parseStatement();
    ASSERT_NOT_NULL(stmt.get());
    ASSERT_EQ(static_cast<int>(stmt->getType()), static_cast<int>(ASTNodeType::CREATE_STMT));
}

TEST(testParserParseDropTable) {
    Lexer lexer("DROP TABLE users;");
    Parser parser(lexer);
    auto stmt = parser.parseStatement();
    ASSERT_NOT_NULL(stmt.get());
    ASSERT_EQ(static_cast<int>(stmt->getType()), static_cast<int>(ASTNodeType::DROP_STMT));
}

TEST(testParserParseDropDatabase) {
    Lexer lexer("DROP DATABASE testdb;");
    Parser parser(lexer);
    auto stmt = parser.parseStatement();
    ASSERT_NOT_NULL(stmt.get());
    ASSERT_EQ(static_cast<int>(stmt->getType()), static_cast<int>(ASTNodeType::DROP_STMT));
}

TEST(testParserParseSelectWithJoin) {
    Lexer lexer("SELECT * FROM users INNER JOIN orders ON users.id = orders.user_id;");
    Parser parser(lexer);
    auto stmt = parser.parseStatement();
    ASSERT_NOT_NULL(stmt.get());
    ASSERT_EQ(static_cast<int>(stmt->getType()), static_cast<int>(ASTNodeType::SELECT_STMT));
}

TEST(testParserParseSelectWithLeftJoin) {
    Lexer lexer("SELECT * FROM users LEFT JOIN orders ON users.id = orders.user_id;");
    Parser parser(lexer);
    auto stmt = parser.parseStatement();
    ASSERT_NOT_NULL(stmt.get());
}

TEST(testParserParseComplexSelect) {
    Lexer lexer("SELECT u.name, o.total FROM users u JOIN orders o ON u.id = o.user_id WHERE o.total > 100;");
    Parser parser(lexer);
    auto stmt = parser.parseStatement();
    ASSERT_NOT_NULL(stmt.get());
}

TEST(testParserParseMultipleJoins) {
    Lexer lexer("SELECT * FROM orders o JOIN customers c ON o.customer_id = c.id JOIN products p ON o.product_id = p.id;");
    Parser parser(lexer);
    auto stmt = parser.parseStatement();
    ASSERT_NOT_NULL(stmt.get());
}

TEST(testParserParseSelectWithAlias) {
    Lexer lexer("SELECT u.name AS username, u.email FROM users u WHERE u.active = 1;");
    Parser parser(lexer);
    auto stmt = parser.parseStatement();
    ASSERT_NOT_NULL(stmt.get());
}

TEST(testParserParseSelectDistinct) {
    Lexer lexer("SELECT DISTINCT category FROM products;");
    Parser parser(lexer);
    auto stmt = parser.parseStatement();
    ASSERT_NOT_NULL(stmt.get());
}

TEST(testParserParseComplexWhere) {
    Lexer lexer("SELECT * FROM orders WHERE (status = 'active' AND total > 100) OR priority = 1;");
    Parser parser(lexer);
    auto stmt = parser.parseStatement();
    ASSERT_NOT_NULL(stmt.get());
}

TEST(testParserParseBatchInsert) {
    Lexer lexer("INSERT INTO users (id, name) VALUES (1, 'Alice'), (2, 'Bob'), (3, 'Charlie');");
    Parser parser(lexer);
    auto stmt = parser.parseStatement();
    ASSERT_NOT_NULL(stmt.get());
    ASSERT_EQ(static_cast<int>(stmt->getType()), static_cast<int>(ASTNodeType::INSERT_STMT));
}

TEST(testParserParseUpdateMultipleColumns) {
    Lexer lexer("UPDATE users SET age = 25, status = 'active', updated_at = NOW() WHERE id = 1;");
    Parser parser(lexer);
    auto stmt = parser.parseStatement();
    ASSERT_NOT_NULL(stmt.get());
}

TEST(testParserParseCreateTableWithConstraints) {
    Lexer lexer("CREATE TABLE users (id INT PRIMARY KEY, name VARCHAR(100) NOT NULL, email VARCHAR(200) UNIQUE);");
    Parser parser(lexer);
    auto stmt = parser.parseStatement();
    ASSERT_NOT_NULL(stmt.get());
    ASSERT_EQ(static_cast<int>(stmt->getType()), static_cast<int>(ASTNodeType::CREATE_STMT));
}

TEST(testParserParseDropIfExists) {
    Lexer lexer("DROP TABLE IF EXISTS temp_table;");
    Parser parser(lexer);
    auto stmt = parser.parseStatement();
    ASSERT_NOT_NULL(stmt.get());
}

TEST(testParserParseEndToEndQuery) {
    // 真实的复杂查询示例
    Lexer lexer("SELECT c.name, COUNT(o.id) AS order_count FROM customers c LEFT JOIN orders o ON c.id = o.customer_id WHERE o.created_at > '2024-01-01' GROUP BY c.id HAVING COUNT(o.id) > 5 ORDER BY order_count DESC LIMIT 10;");
    Parser parser(lexer);
    auto stmt = parser.parseStatement();
    ASSERT_NOT_NULL(stmt.get());
}

// ============================================================
// ALTER TABLE 语句测试
// ============================================================

TEST(testParserParseAlterTableAddColumn) {
    Lexer lexer("ALTER TABLE users ADD COLUMN email VARCHAR(200);");
    Parser parser(lexer);
    auto stmt = parser.parseStatement();
    ASSERT_NOT_NULL(stmt.get());
    ASSERT_EQ(static_cast<int>(stmt->getType()), static_cast<int>(ASTNodeType::ALTER_STMT));
}

TEST(testParserParseAlterTableAddColumnWithoutKeyword) {
    Lexer lexer("ALTER TABLE users ADD email VARCHAR(200);");
    Parser parser(lexer);
    auto stmt = parser.parseStatement();
    ASSERT_NOT_NULL(stmt.get());
}

TEST(testParserParseAlterTableDropColumn) {
    Lexer lexer("ALTER TABLE users DROP COLUMN email;");
    Parser parser(lexer);
    auto stmt = parser.parseStatement();
    ASSERT_NOT_NULL(stmt.get());
}

TEST(testParserParseAlterTableDropColumnWithoutKeyword) {
    Lexer lexer("ALTER TABLE users DROP email;");
    Parser parser(lexer);
    auto stmt = parser.parseStatement();
    ASSERT_NOT_NULL(stmt.get());
}

TEST(testParserParseAlterTableModifyColumn) {
    Lexer lexer("ALTER TABLE users MODIFY COLUMN name VARCHAR(150) NOT NULL;");
    Parser parser(lexer);
    auto stmt = parser.parseStatement();
    ASSERT_NOT_NULL(stmt.get());
}

TEST(testParserParseAlterTableChangeColumn) {
    Lexer lexer("ALTER TABLE users CHANGE COLUMN old_name new_name VARCHAR(100);");
    Parser parser(lexer);
    auto stmt = parser.parseStatement();
    ASSERT_NOT_NULL(stmt.get());
}

TEST(testParserParseAlterTableRenameColumn) {
    Lexer lexer("ALTER TABLE users RENAME COLUMN old_name TO new_name;");
    Parser parser(lexer);
    auto stmt = parser.parseStatement();
    ASSERT_NOT_NULL(stmt.get());
}

TEST(testParserParseAlterTableRenameTable) {
    Lexer lexer("ALTER TABLE users RENAME TO new_users;");
    Parser parser(lexer);
    auto stmt = parser.parseStatement();
    ASSERT_NOT_NULL(stmt.get());
}

TEST(testParserParseAlterTableAddConstraint) {
    Lexer lexer("ALTER TABLE users ADD CONSTRAINT PRIMARY KEY (id);");
    Parser parser(lexer);
    auto stmt = parser.parseStatement();
    ASSERT_NOT_NULL(stmt.get());
}

TEST(testParserParseAlterTableDropConstraint) {
    Lexer lexer("ALTER TABLE users DROP CONSTRAINT fk_test;");
    Parser parser(lexer);
    auto stmt = parser.parseStatement();
    ASSERT_NOT_NULL(stmt.get());
}

// ============================================================
// 函数调用测试
// ============================================================

TEST(testParserParseFunctionCallNoArgs) {
    Lexer lexer("SELECT NOW();");
    Parser parser(lexer);
    auto stmt = parser.parseStatement();
    ASSERT_NOT_NULL(stmt.get());
}

TEST(testParserParseFunctionCallSingleArg) {
    Lexer lexer("SELECT COUNT(id) FROM users;");
    Parser parser(lexer);
    auto stmt = parser.parseStatement();
    ASSERT_NOT_NULL(stmt.get());
}

TEST(testParserParseFunctionCallMultipleArgs) {
    Lexer lexer("SELECT CONCAT(first_name, ' ', last_name) FROM users;");
    Parser parser(lexer);
    auto stmt = parser.parseStatement();
    ASSERT_NOT_NULL(stmt.get());
}

TEST(testParserParseFunctionCallInExpression) {
    Lexer lexer("SELECT * FROM users WHERE YEAR(created_at) = 2024;");
    Parser parser(lexer);
    auto stmt = parser.parseStatement();
    ASSERT_NOT_NULL(stmt.get());
}

TEST(testParserParseFunctionCallNested) {
    Lexer lexer("SELECT UPPER(TRIM(name)) FROM users;");
    Parser parser(lexer);
    auto stmt = parser.parseStatement();
    ASSERT_NOT_NULL(stmt.get());
}

TEST(testParserParseAggregateFunctions) {
    Lexer lexer("SELECT SUM(amount), AVG(price), MIN(quantity), MAX(total) FROM orders;");
    Parser parser(lexer);
    auto stmt = parser.parseStatement();
    ASSERT_NOT_NULL(stmt.get());
}

// ============================================================
// 主函数
// ============================================================

int main() {
    std::cout << "=== Parser Tests ===" << std::endl;

    testParserEmptyInput();
    testParserParseUseStatement();
    testParserParseShowDatabases();
    testParserParseShowTables();
    testParserParseLiteralExpression();
    testParserParseColumnRef();
    testParserParseBinaryExpression();
    testParserParseComparisonExpression();
    testParserParseLogicalExpression();
    testParserParseInsertStatement();
    testParserParseUpdateStatement();
    testParserParseDeleteStatement();
    testParserParseCreateTable();
    testParserParseDropTable();
    testParserParseDropDatabase();
    testParserParseSelectWithJoin();
    testParserParseSelectWithLeftJoin();
    testParserParseComplexSelect();
    testParserParseMultipleJoins();
    testParserParseSelectWithAlias();
    testParserParseSelectDistinct();
    testParserParseComplexWhere();
    testParserParseBatchInsert();
    testParserParseUpdateMultipleColumns();
    testParserParseCreateTableWithConstraints();
    testParserParseDropIfExists();
    testParserParseEndToEndQuery();

    // ALTER TABLE 测试
    testParserParseAlterTableAddColumn();
    testParserParseAlterTableAddColumnWithoutKeyword();
    testParserParseAlterTableDropColumn();
    testParserParseAlterTableDropColumnWithoutKeyword();
    testParserParseAlterTableModifyColumn();
    testParserParseAlterTableChangeColumn();
    testParserParseAlterTableRenameColumn();
    testParserParseAlterTableRenameTable();
    testParserParseAlterTableAddConstraint();
    testParserParseAlterTableDropConstraint();

    // 函数调用测试
    testParserParseFunctionCallNoArgs();
    testParserParseFunctionCallSingleArg();
    testParserParseFunctionCallMultipleArgs();
    testParserParseFunctionCallInExpression();
    testParserParseFunctionCallNested();
    testParserParseAggregateFunctions();

    RUN_TESTS;

    return 0;
}
