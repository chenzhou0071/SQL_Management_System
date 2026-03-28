#pragma once

#include "ExecutionOperator.h"
#include "../common/Type.h"
#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <map>

namespace minisql {

// 前向声明
class TableManager;
struct TableDef;

// 表数据（行 ID -> 行）
using TableData = std::map<int, Row>;

namespace executor {

// 表扫描算子 - 实现全表扫描
class TableScanOperator : public ExecutionOperator {
public:
    // 构造函数
    TableScanOperator(const std::string& dbName, const std::string& tableName);

    // 析构函数
    ~TableScanOperator() override;

    // 初始化算子
    Result<void> open() override;

    // 获取下一行 (返回 nullopt 表示没有更多行)
    Result<std::optional<Tuple>> next() override;

    // 关闭算子
    Result<void> close() override;

    // 获取输出列名
    std::vector<std::string> getColumnNames() const override;

    // 获取输出列类型
    std::vector<DataType> getColumnTypes() const override;

    // 获取表名
    std::string getTableName() const override;

private:
    std::string dbName_;
    std::string tableName_;
    TableDef tableDef_;
    TableData tableData_;
    size_t currentRowIndex_;
    bool isOpen_;
};

} // namespace executor
} // namespace minisql
