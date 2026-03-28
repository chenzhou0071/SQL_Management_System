#pragma once

#include "ExecutionOperator.h"
#include "ExpressionEvaluator.h"
#include "../parser/AST.h"
#include "../common/Type.h"
#include <string>
#include <vector>
#include <memory>
#include <optional>

namespace minisql {
namespace executor {

// 过滤算子 - 实现 WHERE/HAVING 条件执行
class FilterOperator : public ExecutionOperator {
public:
    // 构造函数
    FilterOperator(OperatorPtr child, parser::ExprPtr filterExpr);

    // 析构函数
    ~FilterOperator() override;

    // 初始化算子
    Result<void> open() override;

    // 获取下一行 (只返回满足条件的行)
    Result<std::optional<Tuple>> next() override;

    // 关闭算子
    Result<void> close() override;

    // 获取输出列名
    std::vector<std::string> getColumnNames() const override;

    // 获取输出列类型
    std::vector<DataType> getColumnTypes() const override;

    // 获取表名 (委托给子算子)
    std::string getTableName() const override;

    // 设置当前数据库 (用于子查询)
    void setCurrentDatabase(const std::string& dbName);

private:
    OperatorPtr child_;
    parser::ExprPtr filterExpr_;
    ExpressionEvaluator evaluator_;
    bool isOpen_;
    std::string currentDatabase_;

    // 构建行上下文用于表达式求值
    RowContext buildRowContext(const Tuple& row);
};

} // namespace executor
} // namespace minisql
