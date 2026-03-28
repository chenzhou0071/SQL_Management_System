#pragma once

#include "../common/Type.h"
#include "../common/Value.h"
#include "../common/Error.h"
#include <string>
#include <vector>
#include <map>
#include <memory>

namespace minisql {
namespace optimizer {

// ============================================================
// 列统计信息
// ============================================================
struct ColumnStatistics {
    std::string columnName;
    int64_t distinctValues = 0;    // 不同值数量
    int64_t nullCount = 0;        // NULL 值数量
    double avgLength = 0;          // 平均长度
    Value minValue;                // 最小值
    Value maxValue;                // 最大值
    std::vector<int64_t> histogram;  // 等宽直方图（可选）
};

// ============================================================
// 表统计信息
// ============================================================
struct TableStatistics {
    std::string tableName;
    int64_t rowCount = 0;          // 行数
    int64_t tableSize = 0;         // 表大小（字节）
    std::string lastAnalyzed;      // 最后分析时间
    std::vector<ColumnStatistics> columns;  // 列统计
};

// ============================================================
// 统计信息管理器
// ============================================================
class Statistics {
public:
    static Statistics& getInstance();

    // 分析表并收集统计信息
    Result<void> analyzeTable(const std::string& dbName, const std::string& tableName);

    // 获取表统计信息
    Result<TableStatistics> getTableStatistics(const std::string& dbName, const std::string& tableName);

    // 获取列统计信息
    Result<ColumnStatistics> getColumnStatistics(const std::string& dbName,
                                                  const std::string& tableName,
                                                  const std::string& columnName);

    // 加载/保存统计信息
    Result<void> loadStatistics(const std::string& dbName, const std::string& tableName);
    Result<void> saveStatistics(const std::string& dbName, const std::string& tableName);

    // 估算 selectivity（选择性）
    // selectivity = 1 / distinctValues（等值条件）
    // selectivity = range / total_range（范围条件）
    double estimateSelectivity(const std::string& dbName,
                               const std::string& tableName,
                               const std::string& columnName);

    // 估算行数
    int64_t estimateRowCount(const std::string& dbName,
                             const std::string& tableName,
                             double selectivity);

    // 清除缓存
    void clearCache();

private:
    Statistics() = default;
    ~Statistics() = default;
    Statistics(const Statistics&) = delete;
    Statistics& operator=(const Statistics&) = delete;

    // 获取统计文件路径
    std::string getStatsFilePath(const std::string& dbName, const std::string& tableName);

    // 收集列统计信息
    Result<ColumnStatistics> collectColumnStatistics(const std::string& dbName,
                                                     const std::string& tableName,
                                                     const ColumnDef& columnDef,
                                                     const std::vector<Value>& values);

    // 内存缓存（数据库名 -> 表名 -> 统计信息）
    std::map<std::string, std::map<std::string, TableStatistics>> cache_;
};

}  // namespace optimizer
}  // namespace minisql
