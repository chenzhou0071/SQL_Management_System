#pragma once

#include "ExecutionOperator.h"
#include "../parser/AST.h"
#include "../common/Type.h"
#include <string>
#include <vector>
#include <memory>
#include <optional>

namespace minisql {
namespace executor {

// 子查询扫描算子 - 将子查询的结果作为虚拟表扫描
class SubqueryScanOperator : public ExecutionOperator {
public:
    // 构造函数
    SubqueryScanOperator(OperatorPtr subqueryOp, const std::string& alias);

    // 析构函数
    ~SubqueryScanOperator() override;

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

    // 获取表名（返回别名）
    std::string getTableName() const override;

private:
    OperatorPtr subqueryOp_;  // 子查询执行算子
    std::string alias_;       // 子查询别名
    bool isOpen_;

    // 缓存的列名和类型
    std::vector<std::string> columnNames_;
    std::vector<DataType> columnTypes_;

    // 物化后的子查询结果
    std::vector<Tuple> materializedRows_;
    size_t currentRowIndex_;
};

} // namespace executor
} // namespace minisql
