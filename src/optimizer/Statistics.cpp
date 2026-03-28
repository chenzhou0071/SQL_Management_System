#include "Statistics.h"
#include "../common/Logger.h"
#include "../storage/TableManager.h"
#include "../storage/FileIO.h"
#include "../parser/AST.h"
#include "external/json.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <ctime>
#include <set>

using namespace minisql;
using namespace optimizer;
using json = nlohmann::json;

Statistics& Statistics::getInstance() {
    static Statistics instance;
    return instance;
}

std::string Statistics::getStatsFilePath(const std::string& dbName, const std::string& tableName) {
    return storage::FileIO::join(
        storage::FileIO::getDatabaseDir(dbName),
        tableName + ".stats"
    );
}

Result<void> Statistics::analyzeTable(const std::string& dbName, const std::string& tableName) {
    // 获取表定义
    auto tableDefResult = storage::TableManager::getInstance().getTableDef(dbName, tableName);
    if (tableDefResult.isError()) {
        return Result<void>(tableDefResult.getError());
    }
    auto tableDef = *tableDefResult.getValue();

    // 加载表数据
    auto tableDataResult = storage::TableManager::getInstance().loadTable(dbName, tableName);
    if (tableDataResult.isError()) {
        return Result<void>(tableDataResult.getError());
    }
    auto& tableData = *tableDataResult.getValue();

    TableStatistics stats;
    stats.tableName = tableName;
    stats.rowCount = static_cast<int64_t>(tableData.size());

    // 计算表大小
    int64_t tableSize = 0;
    for (const auto& [rowId, row] : tableData) {
        for (const auto& value : row) {
            if (value.isString()) {
                tableSize += static_cast<int64_t>(value.getString().size());
            } else if (value.isNumeric()) {
                tableSize += 8;  // 估计每列8字节
            }
        }
    }
    stats.tableSize = tableSize;

    // 获取当前时间
    auto now = std::time(nullptr);
    char buf[64];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
    stats.lastAnalyzed = buf;

    // 收集每列的统计信息
    for (const auto& columnDef : tableDef.columns) {
        std::vector<Value> columnValues;
        for (const auto& [rowId, row] : tableData) {
            if (rowId < static_cast<int>(tableDef.columns.size())) {
                columnValues.push_back(row[rowId]);
            }
        }

        auto colStatsResult = collectColumnStatistics(dbName, tableName, columnDef, columnValues);
        if (colStatsResult.isSuccess()) {
            stats.columns.push_back(*colStatsResult.getValue());
        }
    }

    // 保存到缓存
    cache_[dbName][tableName] = stats;

    // 持久化
    return saveStatistics(dbName, tableName);
}

Result<ColumnStatistics> Statistics::collectColumnStatistics(
    const std::string& dbName,
    const std::string& tableName,
    const ColumnDef& columnDef,
    const std::vector<Value>& values) {

    ColumnStatistics stats;
    stats.columnName = columnDef.name;

    if (values.empty()) {
        stats.distinctValues = 0;
        stats.avgLength = 0;
        return Result<ColumnStatistics>(stats);
    }

    // 计算 NULL 数量
    int64_t nullCount = 0;
    std::vector<Value> nonNullValues;
    for (const auto& val : values) {
        if (val.isNull()) {
            nullCount++;
        } else {
            nonNullValues.push_back(val);
        }
    }
    stats.nullCount = nullCount;

    // 计算不同值数量
    std::set<std::string> uniqueStrings;
    for (const auto& val : nonNullValues) {
        uniqueStrings.insert(val.toString());
    }
    stats.distinctValues = static_cast<int64_t>(uniqueStrings.size());

    // 计算平均长度
    int64_t totalLength = 0;
    for (const auto& val : nonNullValues) {
        if (val.isString()) {
            totalLength += static_cast<int64_t>(val.getString().size());
        }
    }
    if (!nonNullValues.empty()) {
        stats.avgLength = static_cast<double>(totalLength) / nonNullValues.size();
    }

    // 计算 min/max（仅对可比较类型）
    if (!nonNullValues.empty() && TypeUtils::isNumeric(columnDef.type)) {
        bool hasMin = false, hasMax = false;
        for (const auto& val : nonNullValues) {
            if (val.isNumeric()) {
                if (!hasMin || val < stats.minValue) {
                    stats.minValue = val;
                    hasMin = true;
                }
                if (!hasMax || val > stats.maxValue) {
                    stats.maxValue = val;
                    hasMax = true;
                }
            }
        }
    }

    return Result<ColumnStatistics>(stats);
}

Result<TableStatistics> Statistics::getTableStatistics(const std::string& dbName, const std::string& tableName) {
    // 先从缓存查找
    auto dbIt = cache_.find(dbName);
    if (dbIt != cache_.end()) {
        auto tableIt = dbIt->second.find(tableName);
        if (tableIt != dbIt->second.end()) {
            return Result<TableStatistics>(tableIt->second);
        }
    }

    // 尝试从文件加载
    auto loadResult = loadStatistics(dbName, tableName);
    if (loadResult.isSuccess()) {
        auto dbIt = cache_.find(dbName);
        if (dbIt != cache_.end()) {
            auto tableIt = dbIt->second.find(tableName);
            if (tableIt != dbIt->second.end()) {
                return Result<TableStatistics>(tableIt->second);
            }
        }
    }

    // 返回空统计
    TableStatistics empty;
    empty.tableName = tableName;
    return Result<TableStatistics>(empty);
}

Result<ColumnStatistics> Statistics::getColumnStatistics(const std::string& dbName,
                                                         const std::string& tableName,
                                                         const std::string& columnName) {
    auto tableStatsResult = getTableStatistics(dbName, tableName);
    if (tableStatsResult.isError()) {
        return Result<ColumnStatistics>(tableStatsResult.getError());
    }

    for (const auto& col : tableStatsResult.getValue()->columns) {
        if (col.columnName == columnName) {
            return Result<ColumnStatistics>(col);
        }
    }

    ColumnStatistics empty;
    empty.columnName = columnName;
    return Result<ColumnStatistics>(empty);
}

Result<void> Statistics::loadStatistics(const std::string& dbName, const std::string& tableName) {
    std::string path = getStatsFilePath(dbName, tableName);

    if (!storage::FileIO::existsFile(path)) {
        return Result<void>();
    }

    try {
        std::string content = storage::FileIO::readFromFile(path);
        json j = json::parse(content);

        TableStatistics stats;
        stats.tableName = j.value("tableName", tableName);
        stats.rowCount = j.value("rowCount", 0);
        stats.tableSize = j.value("tableSize", 0);
        stats.lastAnalyzed = j.value("lastAnalyzed", "");

        if (j.contains("columns")) {
            for (const auto& colJson : j["columns"]) {
                ColumnStatistics col;
                col.columnName = colJson.value("columnName", "");
                col.distinctValues = colJson.value("distinctValues", 0);
                col.nullCount = colJson.value("nullCount", 0);
                col.avgLength = colJson.value("avgLength", 0.0);
                col.minValue = Value::parseFrom(colJson.value("minValue", ""), DataType::NULL_TYPE);
                col.maxValue = Value::parseFrom(colJson.value("maxValue", ""), DataType::NULL_TYPE);
                stats.columns.push_back(col);
            }
        }

        cache_[dbName][tableName] = stats;
        return Result<void>();

    } catch (const std::exception& e) {
        LOG_ERROR("Statistics", "Failed to load statistics: " + std::string(e.what()));
        return Result<void>(MiniSQLException(ErrorCode::STORAGE_FILE_CORRUPTED,
            "Failed to load statistics: " + std::string(e.what())));
    }
}

Result<void> Statistics::saveStatistics(const std::string& dbName, const std::string& tableName) {
    auto dbIt = cache_.find(dbName);
    if (dbIt == cache_.end()) {
        return Result<void>();
    }

    auto tableIt = dbIt->second.find(tableName);
    if (tableIt == dbIt->second.end()) {
        return Result<void>();
    }

    const auto& stats = tableIt->second;
    std::string path = getStatsFilePath(dbName, tableName);

    json j;
    j["tableName"] = stats.tableName;
    j["rowCount"] = stats.rowCount;
    j["tableSize"] = stats.tableSize;
    j["lastAnalyzed"] = stats.lastAnalyzed;

    for (const auto& col : stats.columns) {
        json colJson;
        colJson["columnName"] = col.columnName;
        colJson["distinctValues"] = col.distinctValues;
        colJson["nullCount"] = col.nullCount;
        colJson["avgLength"] = col.avgLength;
        colJson["minValue"] = col.minValue.toString();
        colJson["maxValue"] = col.maxValue.toString();
        j["columns"].push_back(colJson);
    }

    std::string content = j.dump(2);
    if (!storage::FileIO::writeToFile(path, content)) {
        return Result<void>(MiniSQLException(ErrorCode::STORAGE_FILE_IO_ERROR,
            "Failed to save statistics"));
    }

    LOG_INFO("Statistics", "Saved statistics for " + dbName + "." + tableName);
    return Result<void>();
}

double Statistics::estimateSelectivity(const std::string& dbName,
                                       const std::string& tableName,
                                       const std::string& columnName) {
    auto colStatsResult = getColumnStatistics(dbName, tableName, columnName);
    if (colStatsResult.isError() || colStatsResult.getValue()->distinctValues == 0) {
        return 1.0;  // 默认返回1（最差选择性）
    }

    auto& colStats = *colStatsResult.getValue();
    return 1.0 / colStats.distinctValues;
}

int64_t Statistics::estimateRowCount(const std::string& dbName,
                                      const std::string& tableName,
                                      double selectivity) {
    auto tableStatsResult = getTableStatistics(dbName, tableName);
    if (tableStatsResult.isError()) {
        return 0;
    }

    auto& tableStats = *tableStatsResult.getValue();
    return static_cast<int64_t>(tableStats.rowCount * selectivity);
}

void Statistics::clearCache() {
    cache_.clear();
}
