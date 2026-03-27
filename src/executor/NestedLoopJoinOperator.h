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

// 连接算子 - 实现嵌套循环连接
class NestedLoopJoinOperator : public ExecutionOperator {
public:
    enum class JoinType {
        INNER,
        LEFT
    };

    // 构造函数
    NestedLoopJoinOperator(OperatorPtr left, OperatorPtr right,
                          parser::ExprPtr joinCond, JoinType joinType);

    // 析构函数
    ~NestedLoopJoinOperator() override;

    // 初始化算子
    Result<void> open() override;

    // 获取下一行
    Result<std::optional<Tuple>> next() override;

    // 关闭算子
    Result<void> close() override;

    // 获取输出列名
    std::vector<std::string> getColumnNames() const override;

    // 获取输出列类型
    std::vector<DataType> getColumnTypes() const override;

private:
    OperatorPtr left_;
    OperatorPtr right_;
    parser::ExprPtr joinCond_;
    JoinType joinType_;
    ExpressionEvaluator evaluator_;
    bool isOpen_;

    // 连接状态
    std::optional<Tuple> currentLeftRow_;
    std::vector<Tuple> rightRows_;
    size_t rightRowIndex_;

    // 缓存的列名和类型
    std::vector<std::string> columnNames_;
    std::vector<DataType> columnTypes_;

    // 加载右表所有行到内存
    Result<void> loadRightTable();

    // 合并左右行
    Tuple joinRows(const Tuple& left, const Tuple& right);

    // 构建行上下文用于表达式求值
    RowContext buildRowContext(const Tuple& left, const Tuple& right);
};

} // namespace executor
} // namespace minisql
