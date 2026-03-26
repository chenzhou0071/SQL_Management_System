#include "../src/parser/AST.h"
#include "test_utils.h"
#include <iostream>

using namespace minisql;
using namespace parser;

// ============================================================
// AST 基础测试
// ============================================================

TEST(testASTNodeBaseExists) {
    ASTNode node;
    ASSERT_EQ(static_cast<int>(node.getType()), static_cast<int>(ASTNodeType::UNKNOWN));
}

TEST(testExpressionNodeExists) {
    Expression expr;
    ASSERT_EQ(static_cast<int>(expr.getType()), static_cast<int>(ASTNodeType::EXPRESSION));
}

TEST(testSelectStmtExists) {
    SelectStmt stmt;
    ASSERT_EQ(static_cast<int>(stmt.getType()), static_cast<int>(ASTNodeType::SELECT_STMT));
}

TEST(testInsertStmtExists) {
    InsertStmt stmt;
    ASSERT_EQ(static_cast<int>(stmt.getType()), static_cast<int>(ASTNodeType::INSERT_STMT));
}

TEST(testUpdateStmtExists) {
    UpdateStmt stmt;
    ASSERT_EQ(static_cast<int>(stmt.getType()), static_cast<int>(ASTNodeType::UPDATE_STMT));
}

TEST(testDeleteStmtExists) {
    DeleteStmt stmt;
    ASSERT_EQ(static_cast<int>(stmt.getType()), static_cast<int>(ASTNodeType::DELETE_STMT));
}

TEST(testCreateTableStmtExists) {
    CreateTableStmt stmt;
    ASSERT_EQ(static_cast<int>(stmt.getType()), static_cast<int>(ASTNodeType::CREATE_STMT));
}

TEST(testDropStmtExists) {
    DropStmt stmt;
    ASSERT_EQ(static_cast<int>(stmt.getType()), static_cast<int>(ASTNodeType::DROP_STMT));
}

TEST(testUseStmtExists) {
    UseStmt stmt;
    ASSERT_EQ(static_cast<int>(stmt.getType()), static_cast<int>(ASTNodeType::USE_STMT));
}

TEST(testShowStmtExists) {
    ShowStmt stmt;
    ASSERT_EQ(static_cast<int>(stmt.getType()), static_cast<int>(ASTNodeType::SHOW_STMT));
}

// ============================================================
// 主函数
// ============================================================

int main() {
    std::cout << "=== AST Tests ===" << std::endl;

    testASTNodeBaseExists();
    testExpressionNodeExists();
    testSelectStmtExists();
    testInsertStmtExists();
    testUpdateStmtExists();
    testDeleteStmtExists();
    testCreateTableStmtExists();
    testDropStmtExists();
    testUseStmtExists();
    testShowStmtExists();

    RUN_TESTS;

    return 0;
}
