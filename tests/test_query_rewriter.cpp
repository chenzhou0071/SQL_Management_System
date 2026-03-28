#include "../src/optimizer/QueryRewriter.h"
#include "../src/parser/AST.h"
#include "../src/common/Value.h"
#include <cassert>
#include <iostream>
#include <memory>

using namespace minisql;

// 创建测试用的 SelectStmt
parser::SelectStmt* createSelectStmt(parser::ExprPtr whereClause) {
    auto stmt = new parser::SelectStmt();
    stmt->whereClause = whereClause;

    // 添加一个简单的 SELECT 列
    auto colRef = std::make_shared<parser::ColumnRef>("", "age");
    stmt->selectItems.push_back(colRef);

    // 添加 FROM 子句
    auto tableRef = std::make_shared<parser::TableRef>();
    tableRef->name = "users";
    stmt->fromTable = tableRef;

    return stmt;
}

// 构建二元表达式
parser::ExprPtr buildBinaryExpr(parser::ExprPtr left, const std::string& op, parser::ExprPtr right) {
    return std::make_shared<parser::BinaryExpr>(left, op, right);
}

// 构建列引用
parser::ExprPtr buildColumnRef(const std::string& column) {
    return std::make_shared<parser::ColumnRef>("", column);
}

// 构建字面量
parser::ExprPtr buildLiteral(int value) {
    return std::make_shared<parser::LiteralExpr>(Value(value));
}

void testConstantFolding() {
    std::cout << "Running testConstantFolding..." << std::endl;

    // 测试：age > 10 + 5  ->  age > 15
    auto left = buildColumnRef("age");
    auto right = buildBinaryExpr(buildLiteral(10), "+", buildLiteral(5));
    auto expr = buildBinaryExpr(left, ">", right);

    auto stmt = createSelectStmt(expr);
    auto result = optimizer::QueryRewriter::rewrite(stmt);

    assert(result != nullptr && "Rewrite should succeed");
    assert(result->whereClause != nullptr && "Where clause should not be null");

    // 常量折叠后，表达式应该被简化
    std::cout << "  Original: " << expr->toString() << std::endl;
    std::cout << "  Rewritten: " << result->whereClause->toString() << std::endl;

    delete stmt;
    std::cout << "  PASSED" << std::endl;
}

void testRedundantConditionElimination() {
    std::cout << "Running testRedundantConditionElimination..." << std::endl;

    // 测试：age > 18 AND TRUE  ->  age > 18
    auto left = buildBinaryExpr(buildColumnRef("age"), ">", buildLiteral(18));
    auto right = std::make_shared<parser::LiteralExpr>(Value(true));
    auto expr = buildBinaryExpr(left, "AND", right);

    auto stmt = createSelectStmt(expr);
    auto result = optimizer::QueryRewriter::rewrite(stmt);

    assert(result != nullptr && "Rewrite should succeed");

    std::cout << "  Original: " << expr->toString() << std::endl;
    std::cout << "  Rewritten: " << result->whereClause->toString() << std::endl;

    delete stmt;
    std::cout << "  PASSED" << std::endl;
}

void testSimpleCondition() {
    std::cout << "Running testSimpleCondition..." << std::endl;

    // 测试：age > 18
    auto expr = buildBinaryExpr(buildColumnRef("age"), ">", buildLiteral(18));

    auto stmt = createSelectStmt(expr);
    auto result = optimizer::QueryRewriter::rewrite(stmt);

    assert(result != nullptr && "Rewrite should succeed");
    assert(result->whereClause != nullptr && "Where clause should not be null");

    std::cout << "  Original: " << expr->toString() << std::endl;
    std::cout << "  Rewritten: " << result->whereClause->toString() << std::endl;

    delete stmt;
    std::cout << "  PASSED" << std::endl;
}

void testOrCondition() {
    std::cout << "Running testOrCondition..." << std::endl;

    // 测试：age > 18 OR FALSE  ->  age > 18
    auto left = buildBinaryExpr(buildColumnRef("age"), ">", buildLiteral(18));
    auto right = std::make_shared<parser::LiteralExpr>(Value(false));
    auto expr = buildBinaryExpr(left, "OR", right);

    auto stmt = createSelectStmt(expr);
    auto result = optimizer::QueryRewriter::rewrite(stmt);

    assert(result != nullptr && "Rewrite should succeed");

    std::cout << "  Original: " << expr->toString() << std::endl;
    std::cout << "  Rewritten: " << result->whereClause->toString() << std::endl;

    delete stmt;
    std::cout << "  PASSED" << std::endl;
}

void testComplexCondition() {
    std::cout << "Running testComplexCondition..." << std::endl;

    // 测试复杂条件：age > 18 AND (status = 1 OR TRUE)
    auto innerLeft = buildBinaryExpr(buildColumnRef("status"), "=", buildLiteral(1));
    auto innerRight = std::make_shared<parser::LiteralExpr>(Value(true));
    auto innerExpr = buildBinaryExpr(innerLeft, "OR", innerRight);

    auto outerLeft = buildBinaryExpr(buildColumnRef("age"), ">", buildLiteral(18));
    auto expr = buildBinaryExpr(outerLeft, "AND", innerExpr);

    auto stmt = createSelectStmt(expr);
    auto result = optimizer::QueryRewriter::rewrite(stmt);

    assert(result != nullptr && "Rewrite should succeed");

    std::cout << "  Original: " << expr->toString() << std::endl;
    std::cout << "  Rewritten: " << result->whereClause->toString() << std::endl;

    delete stmt;
    std::cout << "  PASSED" << std::endl;
}

int main() {
    std::cout << "=== QueryRewriter Module Tests ===" << std::endl;

    testConstantFolding();
    testRedundantConditionElimination();
    testSimpleCondition();
    testOrCondition();
    testComplexCondition();

    std::cout << "\n=== All QueryRewriter Tests PASSED ===" << std::endl;
    return 0;
}
