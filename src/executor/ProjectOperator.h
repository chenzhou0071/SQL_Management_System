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

// 投影算子 - 实现 SELECT 列表计算
class ProjectOperator : public ExecutionOperator {
public:
    // 构造��数
    ProjectOperator(OperatorPtr child, const std::vector<parser::ExprPtr>& projections);

    // 析构函数
    ~ProjectOperator() override;

    // 初始化算子
    Result<void> open() override;

    // 获取下一行 (返回投影后的行)
    Result<std::optional<Tuple>> next() override;

    // 关闭算子
    Result<void> close() override;

    // 获取输出列名
    std::vector<std::string> getColumnNames() const override;

    // 获取输出列类型
    std::vector<DataType> getColumnTypes() const override;

private:
    OperatorPtr child_;
    std::vector<parser::ExprPtr> projections_;
    ExpressionEvaluator evaluator_;
    bool isOpen_;

    // 缓存的列名和类型
    std::vector<std::string> columnNames_;
    std::vector<DataType> columnTypes_;

    // 构建行上下文用于表达式求值
    RowContext buildRowContext(const Tuple& row);

    // 推导列名
    std::string deriveColumnName(parser::ExprPtr expr);

    // 推导表达式类型
    DataType deriveExpressionType(parser::ExprPtr expr);
};

} // namespace executor
} // namespace minisql
