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

// 排序项
struct SortItem {
    parser::ExprPtr expr;
    bool isAsc;  // true = ASC, false = DESC

    SortItem(parser::ExprPtr e, bool asc) : expr(e), isAsc(asc) {}
};

// 排序算子 - 实现 ORDER BY 执行
class SortOperator : public ExecutionOperator {
public:
    // 构造函数
    SortOperator(OperatorPtr child, const std::vector<SortItem>& orderBy);

    // 析构函数
    ~SortOperator() override;

    // 初始化算子
    Result<void> open() override;

    // 获取下一行 (返回已排序的行)
    Result<std::optional<Tuple>> next() override;

    // 关闭算子
    Result<void> close() override;

    // 获取输出列名
    std::vector<std::string> getColumnNames() const override;

    // 获取输出列类型
    std::vector<DataType> getColumnTypes() const override;

private:
    OperatorPtr child_;
    std::vector<SortItem> orderBy_;
    ExpressionEvaluator evaluator_;
    bool isOpen_;

    std::vector<Tuple> sortedRows_;
    size_t currentRowIndex_;

    // 缓存的列名和类型
    std::vector<std::string> columnNames_;
    std::vector<DataType> columnTypes_;

    // 物化并排序所有行
    Result<void> materializeAndSort();

    // 比较两行
    bool compareRows(const Tuple& left, const Tuple& right);

    // 构建行上下文用于表达式求值
    RowContext buildRowContext(const Tuple& row);
};

} // namespace executor
} // namespace minisql
