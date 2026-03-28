#pragma once

#include "../common/Value.h"
#include "../common/Error.h"
#include "../common/Type.h"
#include <vector>
#include <memory>
#include <optional>

namespace minisql {
namespace executor {

// 执行结果元组
using Tuple = std::vector<Value>;

// 执行结果 (带元数据)
struct ExecutionResult {
    std::vector<std::string> columnNames;  // 列名
    std::vector<DataType> columnTypes;      // 列类型
    std::vector<Tuple> rows;                // 数据行

    size_t getRowCount() const;
    size_t getColumnCount() const;
};

// 抽象执行算子 (火山模型迭代器)
class ExecutionOperator {
public:
    virtual ~ExecutionOperator() = default;

    // 初始化算子
    virtual Result<void> open() = 0;

    // 获取下一行 (返回 nullopt 表示没有更多行)
    virtual Result<std::optional<Tuple>> next() = 0;

    // 关闭算子
    virtual Result<void> close() = 0;

    // 获取输出列名
    virtual std::vector<std::string> getColumnNames() const = 0;

    // 获取输出列类型
    virtual std::vector<DataType> getColumnTypes() const = 0;

    // 获取表名 (用于 JOIN 条件中的列名解析)
    virtual std::string getTableName() const = 0;

    // 获取子算子 (如果有)
    virtual std::vector<std::shared_ptr<ExecutionOperator>> getChildren() const {
        return children_;
    }

protected:
    std::vector<std::shared_ptr<ExecutionOperator>> children_;
};

// 执行算子智能指针
using OperatorPtr = std::shared_ptr<ExecutionOperator>;

} // namespace executor
} // namespace minisql
