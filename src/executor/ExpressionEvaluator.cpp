#include "ExpressionEvaluator.h"
#include "../common/Logger.h"
#include <algorithm>
#include <cmath>
#include <ctime>

namespace minisql {
namespace executor {

ExpressionEvaluator::ExpressionEvaluator() {
}

void ExpressionEvaluator::setRowContext(const RowContext& context) {
    rowContext_ = context;
}

Result<Value> ExpressionEvaluator::evaluate(parser::ExprPtr expr) {
    if (!expr) {
        MiniSQLException error(ErrorCode::EXEC_INVALID_VALUE, "NULL expression");
        return Result<Value>(error);
    }

    switch (expr->getType()) {
        case parser::ASTNodeType::LITERAL_EXPR:
            return evaluateLiteral(static_cast<parser::LiteralExpr*>(expr.get()));

        case parser::ASTNodeType::COLUMN_REF:
            return evaluateColumnRef(static_cast<parser::ColumnRef*>(expr.get()));

        case parser::ASTNodeType::BINARY_EXPR:
            return evaluateBinary(static_cast<parser::BinaryExpr*>(expr.get()));

        case parser::ASTNodeType::UNARY_EXPR:
            return evaluateUnary(static_cast<parser::UnaryExpr*>(expr.get()));

        case parser::ASTNodeType::FUNCTION_CALL:
            return evaluateFunction(static_cast<parser::FunctionCallExpr*>(expr.get()));

        default:
            MiniSQLException error(ErrorCode::EXEC_INVALID_VALUE, "Unknown expression type");
            return Result<Value>(error);
    }
}

Result<Value> ExpressionEvaluator::evaluateLiteral(parser::LiteralExpr* expr) {
    if (!expr) {
        MiniSQLException error(ErrorCode::EXEC_INVALID_VALUE, "NULL literal expression");
        return Result<Value>(error);
    }
    return Result<Value>(expr->getValue());
}

Result<Value> ExpressionEvaluator::evaluateColumnRef(parser::ColumnRef* expr) {
    if (!expr) {
        MiniSQLException error(ErrorCode::EXEC_INVALID_VALUE, "NULL column reference");
        return Result<Value>(error);
    }

    std::string colName = expr->getColumn();

    // 在行上下文中查找列值
    auto it = rowContext_.find(colName);
    if (it == rowContext_.end()) {
        MiniSQLException error(ErrorCode::EXEC_COLUMN_NOT_FOUND, "Column not found: " + colName);
        return Result<Value>(error);
    }

    return Result<Value>(it->second);
}

Result<Value> ExpressionEvaluator::evaluateBinary(parser::BinaryExpr* expr) {
    if (!expr) {
        MiniSQLException error(ErrorCode::EXEC_INVALID_VALUE, "NULL binary expression");
        return Result<Value>(error);
    }

    // 递归求值左右操作数
    auto leftResult = evaluate(expr->getLeft());
    if (!leftResult.isSuccess()) {
        return leftResult;
    }

    auto rightResult = evaluate(expr->getRight());
    if (!rightResult.isSuccess()) {
        return rightResult;
    }

    const Value& left = *leftResult.getValue();
    const Value& right = *rightResult.getValue();
    const std::string& op = expr->getOp();

    // 根据操作符类型分发
    if (op == "+" || op == "-" || op == "*" || op == "/") {
        return evaluateArithmetic(left, op, right);
    } else if (op == "AND" || op == "OR") {
        return evaluateLogical(left, op, right);
    } else if (op == "=" || op == "!=" || op == "<" || op == ">" || op == "<=" || op == ">=") {
        return evaluateComparison(left, op, right);
    }

    MiniSQLException error(ErrorCode::EXEC_INVALID_VALUE, "Unknown binary operator: " + op);
    return Result<Value>(error);
}

Result<Value> ExpressionEvaluator::evaluateUnary(parser::UnaryExpr* expr) {
    if (!expr) {
        MiniSQLException error(ErrorCode::EXEC_INVALID_VALUE, "NULL unary expression");
        return Result<Value>(error);
    }

    auto operandResult = evaluate(expr->getOperand());
    if (!operandResult.isSuccess()) {
        return operandResult;
    }

    const Value& operand = *operandResult.getValue();
    const std::string& op = expr->getOp();

    if (op == "NOT") {
        // 逻辑非
        if (operand.isNull()) {
            return Result<Value>(Value()); // NULL
        }
        bool val = !operand.getBool();
        return Result<Value>(Value(val));
    } else if (op == "-") {
        // 算术负号
        if (operand.isNull()) {
            return Result<Value>(Value()); // NULL
        }
        if (operand.getType() == DataType::INT) {
            return Result<Value>(Value(-operand.getInt()));
        } else if (operand.getType() == DataType::DOUBLE) {
            return Result<Value>(Value(-operand.getDouble()));
        }
    }

    MiniSQLException error(ErrorCode::EXEC_INVALID_VALUE, "Unknown unary operator: " + op);
    return Result<Value>(error);
}

Result<Value> ExpressionEvaluator::evaluateFunction(parser::FunctionCallExpr* expr) {
    if (!expr) {
        MiniSQLException error(ErrorCode::EXEC_INVALID_VALUE, "NULL function call");
        return Result<Value>(error);
    }

    // 求值所有参数
    std::vector<Value> args;
    for (auto& arg : expr->getArgs()) {
        auto argResult = evaluate(arg);
        if (!argResult.isSuccess()) {
            return argResult;
        }
        args.push_back(*argResult.getValue());
    }

    const std::string& funcName = expr->getFuncName();

    // 判断是聚合函数还是标量函数
    if (funcName == "COUNT" || funcName == "SUM" || funcName == "AVG" ||
        funcName == "MAX" || funcName == "MIN") {
        return evaluateAggregate(funcName, args);
    } else {
        return evaluateScalarFunction(funcName, args);
    }
}

Result<Value> ExpressionEvaluator::evaluateArithmetic(const Value& left, const std::string& op, const Value& right) {
    // NULL 处理
    if (left.isNull() || right.isNull()) {
        return Result<Value>(Value()); // NULL
    }

    // 类型提升: 如果任一操作数是 DOUBLE，结果为 DOUBLE
    if (left.getType() == DataType::DOUBLE || right.getType() == DataType::DOUBLE) {
        double l = (left.getType() == DataType::INT) ? left.getInt() : left.getDouble();
        double r = (right.getType() == DataType::INT) ? right.getInt() : right.getDouble();
        double result = 0.0;

        if (op == "+") result = l + r;
        else if (op == "-") result = l - r;
        else if (op == "*") result = l * r;
        else if (op == "/") {
            if (r == 0.0) {
                MiniSQLException error(ErrorCode::EXEC_DIVISION_BY_ZERO, "Division by zero");
                return Result<Value>(error);
            }
            result = l / r;
        }

        return Result<Value>(Value(result));
    } else {
        // 整数运算
        int64_t l = left.getInt();
        int64_t r = right.getInt();
        int64_t result = 0;

        if (op == "+") result = l + r;
        else if (op == "-") result = l - r;
        else if (op == "*") result = l * r;
        else if (op == "/") {
            if (r == 0) {
                MiniSQLException error(ErrorCode::EXEC_DIVISION_BY_ZERO, "Division by zero");
                return Result<Value>(error);
            }
            result = l / r;
        }

        return Result<Value>(Value(result));
    }
}

Result<Value> ExpressionEvaluator::evaluateLogical(const Value& left, const std::string& op, const Value& right) {
    // NULL 在布尔上下文中视为 FALSE
    bool l = left.isNull() ? false : left.getBool();
    bool r = right.isNull() ? false : right.getBool();
    bool result = false;

    if (op == "AND") {
        result = l && r;
    } else if (op == "OR") {
        result = l || r;
    }

    return Result<Value>(Value(result));
}

Result<Value> ExpressionEvaluator::evaluateComparison(const Value& left, const std::string& op, const Value& right) {
    // NULL 处理
    if (left.isNull() || right.isNull()) {
        return Result<Value>(Value()); // NULL
    }

    int cmp = left.compareTo(right);
    bool result = false;

    if (op == "=") result = (cmp == 0);
    else if (op == "!=") result = (cmp != 0);
    else if (op == "<") result = (cmp < 0);
    else if (op == ">") result = (cmp > 0);
    else if (op == "<=") result = (cmp <= 0);
    else if (op == ">=") result = (cmp >= 0);

    return Result<Value>(Value(result));
}

Result<Value> ExpressionEvaluator::evaluateAggregate(const std::string& funcName, const std::vector<Value>& args) {
    // 聚合函数在 AggregateOperator 中处理
    // 这里只处理简单的常量聚合
    MiniSQLException error(ErrorCode::EXEC_INVALID_VALUE,
        "Aggregate functions should be handled by AggregateOperator: " + funcName);
    return Result<Value>(error);
}

Result<Value> ExpressionEvaluator::evaluateScalarFunction(const std::string& funcName, const std::vector<Value>& args) {
    // 字符串函数
    if (funcName == "CONCAT") {
        if (args.size() < 2) {
            MiniSQLException error(ErrorCode::EXEC_INVALID_VALUE, "CONCAT requires at least 2 arguments");
            return Result<Value>(error);
        }
        std::string result = "";
        for (const auto& arg : args) {
            if (arg.isNull()) {
                return Result<Value>(Value()); // NULL
            }
            result += arg.getString();
        }
        return Result<Value>(Value(result));
    }
    else if (funcName == "UPPER") {
        if (args.size() != 1) {
            MiniSQLException error(ErrorCode::EXEC_INVALID_VALUE, "UPPER requires exactly 1 argument");
            return Result<Value>(error);
        }
        if (args[0].isNull()) {
            return Result<Value>(Value());
        }
        std::string str = args[0].getString();
        std::transform(str.begin(), str.end(), str.begin(), ::toupper);
        return Result<Value>(Value(str));
    }
    else if (funcName == "LOWER") {
        if (args.size() != 1) {
            MiniSQLException error(ErrorCode::EXEC_INVALID_VALUE, "LOWER requires exactly 1 argument");
            return Result<Value>(error);
        }
        if (args[0].isNull()) {
            return Result<Value>(Value());
        }
        std::string str = args[0].getString();
        std::transform(str.begin(), str.end(), str.begin(), ::tolower);
        return Result<Value>(Value(str));
    }
    // 日期函数
    else if (funcName == "NOW") {
        // 返回当前时间戳
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        std::string timeStr = std::ctime(&time);
        if (!timeStr.empty() && timeStr.back() == '\n') {
            timeStr.pop_back(); // 移除换行符
        }
        return Result<Value>(Value(timeStr));
    }
    else if (funcName == "LENGTH") {
        if (args.size() != 1) {
            MiniSQLException error(ErrorCode::EXEC_INVALID_VALUE, "LENGTH requires exactly 1 argument");
            return Result<Value>(error);
        }
        if (args[0].isNull()) {
            return Result<Value>(Value());
        }
        int64_t len = static_cast<int64_t>(args[0].getString().length());
        return Result<Value>(Value(len));
    }
    else if (funcName == "ROUND") {
        if (args.size() < 1 || args.size() > 2) {
            MiniSQLException error(ErrorCode::EXEC_INVALID_VALUE, "ROUND requires 1 or 2 arguments");
            return Result<Value>(error);
        }
        if (args[0].isNull()) {
            return Result<Value>(Value());
        }
        int decimals = (args.size() > 1 && !args[1].isNull()) ?
            static_cast<int>(args[1].getInt()) : 0;

        if (args[0].getType() == DataType::DOUBLE) {
            double val = args[0].getDouble();
            double multiplier = 1.0;
            for (int i = 0; i < decimals; ++i) multiplier *= 10.0;
            double result = ::round(val * multiplier) / multiplier;
            return Result<Value>(Value(result));
        } else {
            int64_t val = args[0].getInt();
            return Result<Value>(Value(val));
        }
    }
    else if (funcName == "ABS") {
        if (args.size() != 1) {
            MiniSQLException error(ErrorCode::EXEC_INVALID_VALUE, "ABS requires exactly 1 argument");
            return Result<Value>(error);
        }
        if (args[0].isNull()) {
            return Result<Value>(Value());
        }
        if (args[0].getType() == DataType::DOUBLE) {
            return Result<Value>(Value(std::abs(args[0].getDouble())));
        } else {
            return Result<Value>(Value(std::abs(args[0].getInt())));
        }
    }

    MiniSQLException error(ErrorCode::EXEC_INVALID_VALUE, "Unknown scalar function: " + funcName);
    return Result<Value>(error);
}

} // namespace executor
} // namespace minisql
