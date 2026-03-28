#pragma once

#include "../parser/AST.h"
#include "../common/Value.h"
#include "../common/Error.h"
#include "../common/Type.h"
#include "../storage/TableManager.h"
#include <string>
#include <map>
#include <memory>
#include <vector>

namespace minisql {
namespace executor {

// 行上下文 (列名 -> 值)
using RowContext = std::map<std::string, Value>;

class ExpressionEvaluator {
public:
    ExpressionEvaluator();

    // 求值表达式
    Result<Value> evaluate(parser::ExprPtr expr);

    // 设置当前行上下文 (用于列引用)
    void setRowContext(const RowContext& context);

    // 设置当前数据库 (用于子查询)
    void setCurrentDatabase(const std::string& dbName) { currentDatabase_ = dbName; }

    // 获取当前行上下文
    const RowContext& getRowContext() const { return rowContext_; }

private:
    // 字面量求值
    Result<Value> evaluateLiteral(parser::LiteralExpr* expr);

    // 列引用求值
    Result<Value> evaluateColumnRef(parser::ColumnRef* expr);

    // 二元表达式求值
    Result<Value> evaluateBinary(parser::BinaryExpr* expr);

    // 一元表达式求值
    Result<Value> evaluateUnary(parser::UnaryExpr* expr);

    // 函数调用求值
    Result<Value> evaluateFunction(parser::FunctionCallExpr* expr);

    // 算术运算
    Result<Value> evaluateArithmetic(const Value& left, const std::string& op, const Value& right);

    // 逻辑运算
    Result<Value> evaluateLogical(const Value& left, const std::string& op, const Value& right);

    // 比较运算
    Result<Value> evaluateComparison(const Value& left, const std::string& op, const Value& right);

    // 聚合函数求值 (在 AggregateOperator 中处理)
    Result<Value> evaluateAggregate(const std::string& funcName, const std::vector<Value>& args);

    // 标量函数求值
    Result<Value> evaluateScalarFunction(const std::string& funcName, const std::vector<Value>& args);

    // IN 子查询求值
    Result<Value> evaluateInSubquery(parser::InSubqueryExpr* expr);

    // EXISTS 子查询求值
    Result<Value> evaluateExists(parser::ExistsExpr* expr);

    // 执行子查询并返回结果
    Result<std::vector<Value>> executeSubquery(parser::SelectStmt* subquery);

    RowContext rowContext_;
    std::string currentDatabase_;  // 用于子查询执行
};

} // namespace executor
} // namespace minisql
