#pragma once

#include "ExecutionOperator.h"
#include "../common/Type.h"
#include <string>
#include <vector>
#include <memory>
#include <optional>

namespace minisql {
namespace executor {

// 限制算子 - 实现 LIMIT/OFFSET 执行
class LimitOperator : public ExecutionOperator {
public:
    // 构造函数
    LimitOperator(OperatorPtr child, size_t limit, size_t offset = 0);

    // 析构函数
    ~LimitOperator() override;

    // 初始化算子
    Result<void> open() override;

    // 获取下一行 (应用 LIMIT 和 OFFSET)
    Result<std::optional<Tuple>> next() override;

    // 关闭算子
    Result<void> close() override;

    // 获取输出列名
    std::vector<std::string> getColumnNames() const override;

    // 获取输出列类型
    std::vector<DataType> getColumnTypes() const override;

private:
    OperatorPtr child_;
    size_t limit_;      // 限制返回的行数
    size_t offset_;     // 跳过的行数
    bool isOpen_;

    size_t skipCount_;   // 已跳过的行数
    size_t produceCount_; // 已产生的行数
};

} // namespace executor
} // namespace minisql
