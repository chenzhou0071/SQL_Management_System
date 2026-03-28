#include "QueryRewriter.h"
#include "../common/Logger.h"
#include "../executor/ExpressionEvaluator.h"
#include <algorithm>

using namespace minisql;
using namespace optimizer;

std::shared_ptr<parser::SelectStmt> QueryRewriter::rewrite(parser::SelectStmt* stmt) {
    if (!stmt) {
        return nullptr;
    }

    // 复制语句以避免修改原语句
    auto rewritten = std::make_shared<parser::SelectStmt>(*stmt);

    // 1. 谓词下推
    LOG_DEBUG("QueryRewriter", "Step 1: pushDownPredicates");
    auto pushResult = pushDownPredicates(rewritten.get());
    if (pushResult.isError()) {
        return nullptr;
    }

    // 2. 消除冗余条件
    LOG_DEBUG("QueryRewriter", "Step 2: eliminateRedundantConditions");
    if (rewritten->whereClause) {
        auto elimResult = eliminateRedundantConditions(rewritten->whereClause);
        if (elimResult.isSuccess()) {
            rewritten->whereClause = *elimResult.getValue();
        }
    }

    // 3. 常量折叠
    LOG_DEBUG("QueryRewriter", "Step 3: foldConstants");
    if (rewritten->whereClause) {
        auto foldResult = foldConstants(rewritten->whereClause);
        if (foldResult.isSuccess()) {
            rewritten->whereClause = *foldResult.getValue();
        }
    }

    // 4. 简化布尔表达式
    LOG_DEBUG("QueryRewriter", "Step 4: simplifyBooleanExpr");
    if (rewritten->whereClause) {
        auto simpResult = simplifyBooleanExpr(rewritten->whereClause);
        if (simpResult.isSuccess()) {
            rewritten->whereClause = *simpResult.getValue();
        }
    }

    LOG_INFO("QueryRewriter", "Query rewritten successfully");
    return rewritten;
}

Result<parser::ExprPtr> QueryRewriter::foldConstants(parser::ExprPtr expr) {
    if (!expr) {
        return Result<parser::ExprPtr>(expr);
    }

    // 处理 IN_SUBQUERY_EXPR 和 EXISTS_EXPR - 直接返回
    if (expr->getType() == parser::ASTNodeType::IN_SUBQUERY_EXPR ||
        expr->getType() == parser::ASTNodeType::EXISTS_EXPR) {
        return Result<parser::ExprPtr>(expr);
    }

    // 处理二元表达式
    if (expr->getType() == parser::ASTNodeType::BINARY_EXPR) {
        auto binExpr = std::dynamic_pointer_cast<parser::BinaryExpr>(expr);
        if (binExpr) {
            // 递归处理左右操作数
            Result<parser::ExprPtr> leftResult = foldConstants(binExpr->getLeft());
            Result<parser::ExprPtr> rightResult = foldConstants(binExpr->getRight());

            if (leftResult.isSuccess() && rightResult.isSuccess()) {
                auto left = *leftResult.getValue();
                auto right = *rightResult.getValue();

                // 检查是否都是字面量（常量）
                if (left && left->getType() == parser::ASTNodeType::LITERAL_EXPR &&
                    right && right->getType() == parser::ASTNodeType::LITERAL_EXPR) {

                    auto leftLit = std::dynamic_pointer_cast<parser::LiteralExpr>(left);
                    auto rightLit = std::dynamic_pointer_cast<parser::LiteralExpr>(right);

                    if (leftLit && rightLit) {
                        std::string op = binExpr->getOp();
                        std::transform(op.begin(), op.end(), op.begin(), ::toupper);

                        // 尝试进行常量计算
                        try {
                            if (op == "+" || op == "-" || op == "*" || op == "/") {
                                // 数值运算
                                auto leftVal = leftLit->getValue();
                                auto rightVal = rightLit->getValue();

                                if (leftVal.isNumeric() && rightVal.isNumeric()) {
                                    Value result;
                                    if (op == "+") {
                                        result = Value(leftVal.getInt() + rightVal.getInt());
                                    } else if (op == "-") {
                                        result = Value(leftVal.getInt() - rightVal.getInt());
                                    } else if (op == "*") {
                                        result = Value(leftVal.getInt() * rightVal.getInt());
                                    } else if (op == "/") {
                                        if (rightVal.getInt() != 0) {
                                            result = Value(leftVal.getInt() / rightVal.getInt());
                                        }
                                    }
                                    return Result<parser::ExprPtr>(
                                        std::make_shared<parser::LiteralExpr>(result));
                                }
                            }
                        } catch (...) {
                            // 计算失败，返回原表达式
                        }
                    }
                }

                // 返回带有折叠后操作数的新表达式
                auto newExpr = std::make_shared<parser::BinaryExpr>(
                    left, binExpr->getOp(), right);
                return Result<parser::ExprPtr>(newExpr);
            }
        }
    }

    // 处理一元表达式
    if (expr->getType() == parser::ASTNodeType::UNARY_EXPR) {
        auto unaryExpr = std::dynamic_pointer_cast<parser::UnaryExpr>(expr);
        if (unaryExpr) {
            auto operandResult = foldConstants(unaryExpr->getOperand());
            if (operandResult.isSuccess() && operandResult.getValue()) {
                auto newExpr = std::make_shared<parser::UnaryExpr>(
                    unaryExpr->getOp(), *operandResult.getValue());
                return Result<parser::ExprPtr>(newExpr);
            }
        }
    }

    return Result<parser::ExprPtr>(expr);
}

Result<parser::ExprPtr> QueryRewriter::eliminateRedundantConditions(parser::ExprPtr expr) {
    if (!expr) {
        return Result<parser::ExprPtr>(expr);
    }

    // 处理 IN_SUBQUERY_EXPR 和 EXISTS_EXPR - 直接返回
    if (expr->getType() == parser::ASTNodeType::IN_SUBQUERY_EXPR ||
        expr->getType() == parser::ASTNodeType::EXISTS_EXPR) {
        return Result<parser::ExprPtr>(expr);
    }

    if (expr->getType() == parser::ASTNodeType::BINARY_EXPR) {
        auto binExpr = std::dynamic_pointer_cast<parser::BinaryExpr>(expr);
        if (binExpr) {
            std::string op = binExpr->getOp();
            std::transform(op.begin(), op.end(), op.begin(), ::toupper);

            if (op == "AND") {
                // 递归处理 AND 的左右分支
                auto leftResult = eliminateRedundantConditions(binExpr->getLeft());
                auto rightResult = eliminateRedundantConditions(binExpr->getRight());

                if (leftResult.isSuccess() && rightResult.isSuccess()) {
                    auto left = *leftResult.getValue();
                    auto right = *rightResult.getValue();

                    // 如果任一边恒为 false，整个表达式为 false
                    if (isAlwaysFalse(left) || isAlwaysFalse(right)) {
                        return Result<parser::ExprPtr>(
                            std::make_shared<parser::LiteralExpr>(Value(false)));
                    }

                    // 如果一边恒为 true，直接返回另一边
                    if (isAlwaysTrue(left)) {
                        return Result<parser::ExprPtr>(right);
                    }
                    if (isAlwaysTrue(right)) {
                        return Result<parser::ExprPtr>(left);
                    }

                    // 如果两边相同，只保留一个
                    if (left && right && left->toString() == right->toString()) {
                        return Result<parser::ExprPtr>(left);
                    }

                    return Result<parser::ExprPtr>(
                        std::make_shared<parser::BinaryExpr>(left, binExpr->getOp(), right));
                }
            } else if (op == "OR") {
                auto leftResult = eliminateRedundantConditions(binExpr->getLeft());
                auto rightResult = eliminateRedundantConditions(binExpr->getRight());

                if (leftResult.isSuccess() && rightResult.isSuccess()) {
                    auto left = *leftResult.getValue();
                    auto right = *rightResult.getValue();

                    // 如果任一边恒为 true，整个表达式为 true
                    if (isAlwaysTrue(left) || isAlwaysTrue(right)) {
                        return Result<parser::ExprPtr>(
                            std::make_shared<parser::LiteralExpr>(Value(true)));
                    }

                    // 如果一边恒为 false，直接返回另一边
                    if (isAlwaysFalse(left)) {
                        return Result<parser::ExprPtr>(right);
                    }
                    if (isAlwaysFalse(right)) {
                        return Result<parser::ExprPtr>(left);
                    }

                    // 简化：age > 18 OR age = 20 -> age >= 18
                    // 这里需要更复杂的逻辑，简单版本只处理完全相同的情况
                    if (left && right && left->toString() == right->toString()) {
                        return Result<parser::ExprPtr>(left);
                    }

                    return Result<parser::ExprPtr>(
                        std::make_shared<parser::BinaryExpr>(left, binExpr->getOp(), right));
                }
            }
        }
    }

    return Result<parser::ExprPtr>(expr);
}

Result<void> QueryRewriter::pushDownPredicates(parser::SelectStmt* stmt) {
    // 简单的谓词下推：
    // 如果 WHERE 条件可以下推到 JOIN，则进行下推
    // 目前实现基本的列裁剪优化

    if (!stmt || !stmt->whereClause) {
        return Result<void>();
    }

    // 对于嵌套查询，可以将 WHERE 条件下推到子查询
    // 这里暂时不做复杂实现
    return Result<void>();
}

Result<parser::ExprPtr> QueryRewriter::simplifyBooleanExpr(parser::ExprPtr expr) {
    if (!expr) {
        return Result<parser::ExprPtr>(expr);
    }

    // 处理 IN_SUBQUERY_EXPR 和 EXISTS_EXPR - 直接返回，不递归处理
    if (expr->getType() == parser::ASTNodeType::IN_SUBQUERY_EXPR ||
        expr->getType() == parser::ASTNodeType::EXISTS_EXPR) {
        return Result<parser::ExprPtr>(expr);
    }

    // 处理二元表达式
    if (expr->getType() == parser::ASTNodeType::BINARY_EXPR) {
        auto binExpr = std::dynamic_pointer_cast<parser::BinaryExpr>(expr);
        if (binExpr) {
            std::string op = binExpr->getOp();
            std::transform(op.begin(), op.end(), op.begin(), ::toupper);

            if (op == "AND" || op == "OR") {
                auto leftResult = simplifyBooleanExpr(binExpr->getLeft());
                auto rightResult = simplifyBooleanExpr(binExpr->getRight());

                if (leftResult.isSuccess() && rightResult.isSuccess()) {
                    auto left = *leftResult.getValue();
                    auto right = *rightResult.getValue();

                    return Result<parser::ExprPtr>(
                        std::make_shared<parser::BinaryExpr>(left, binExpr->getOp(), right));
                }
            }
        }
    }

    return Result<parser::ExprPtr>(expr);
}

bool QueryRewriter::isAlwaysTrue(parser::ExprPtr expr) {
    if (!expr) return false;

    if (expr->getType() == parser::ASTNodeType::LITERAL_EXPR) {
        auto lit = std::dynamic_pointer_cast<parser::LiteralExpr>(expr);
        if (lit) {
            return lit->getValue().getBool() == true;
        }
    }

    return false;
}

bool QueryRewriter::isAlwaysFalse(parser::ExprPtr expr) {
    if (!expr) return false;

    if (expr->getType() == parser::ASTNodeType::LITERAL_EXPR) {
        auto lit = std::dynamic_pointer_cast<parser::LiteralExpr>(expr);
        if (lit) {
            return lit->getValue().getBool() == false;
        }
    }

    return false;
}

std::string QueryRewriter::extractColumnName(parser::ExprPtr expr) {
    if (!expr) return "";

    if (expr->getType() == parser::ASTNodeType::COLUMN_REF) {
        auto colRef = std::dynamic_pointer_cast<parser::ColumnRef>(expr);
        if (colRef) {
            return colRef->getColumn();
        }
    }

    return "";
}
