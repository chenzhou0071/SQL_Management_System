#include "../src/optimizer/Statistics.h"
#include "../src/storage/TableManager.h"
#include "../src/storage/IndexManager.h"
#include "../src/common/Type.h"
#include "../src/common/Value.h"
#include <cassert>
#include <iostream>

using namespace minisql;

void setupTestTable() {
    auto& tableMgr = storage::TableManager::getInstance();

    // 创建测试数据库
    if (tableMgr.databaseExists("test_stats_db")) {
        tableMgr.dropDatabase("test_stats_db");
    }
    tableMgr.createDatabase("test_stats_db");

    // 创建表
    TableDef tableDef;
    tableDef.name = "users";
    tableDef.database = "test_stats_db";

    ColumnDef idCol;
    idCol.name = "id";
    idCol.type = DataType::INT;
    idCol.primaryKey = true;

    ColumnDef ageCol;
    ageCol.name = "age";
    ageCol.type = DataType::INT;

    ColumnDef nameCol;
    nameCol.name = "name";
    nameCol.type = DataType::VARCHAR;
    nameCol.length = 50;

    ColumnDef emailCol;
    emailCol.name = "email";
    emailCol.type = DataType::VARCHAR;
    emailCol.length = 100;

    tableDef.columns.push_back(idCol);
    tableDef.columns.push_back(ageCol);
    tableDef.columns.push_back(nameCol);
    tableDef.columns.push_back(emailCol);

    // 添加索引
    IndexDef idxAge;
    idxAge.name = "idx_age";
    idxAge.tableName = "users";
    idxAge.columns.push_back("age");
    tableDef.indexes.push_back(idxAge);

    tableMgr.createTable("test_stats_db", tableDef);

    // 插入测试数据
    for (int i = 1; i <= 100; ++i) {
        int age = 18 + (i % 50);  // 年龄分布 18-67
        std::string name = "User" + std::to_string(i);
        std::string email = "user" + std::to_string(i) + "@example.com";
        tableMgr.insertRow("test_stats_db", "users",
            {Value(i), Value(age), Value(name), Value(email)});
    }
}

void testAnalyzeTable() {
    std::cout << "Running testAnalyzeTable..." << std::endl;

    auto& stats = optimizer::Statistics::getInstance();

    // 分析表
    auto result = stats.analyzeTable("test_stats_db", "users");
    assert(result.isSuccess() && "analyzeTable should succeed");

    // 获取统计信息
    auto tableStats = stats.getTableStatistics("test_stats_db", "users");
    assert(tableStats.isSuccess() && "getTableStatistics should succeed");
    assert(tableStats.getValue()->rowCount == 100 && "Should have 100 rows");
    assert(!tableStats.getValue()->lastAnalyzed.empty() && "Should have analyze time");

    std::cout << "  PASSED: rowCount = " << tableStats.getValue()->rowCount << std::endl;
}

void testColumnStatistics() {
    std::cout << "Running testColumnStatistics..." << std::endl;

    auto& stats = optimizer::Statistics::getInstance();

    auto colStats = stats.getColumnStatistics("test_stats_db", "users", "age");
    assert(colStats.isSuccess() && "getColumnStatistics should succeed");

    // age 列应该有 distinct values
    assert(colStats.getValue()->distinctValues > 0 && "Should have distinct age values");

    std::cout << "  PASSED: distinctValues = " << colStats.getValue()->distinctValues << std::endl;
}

void testSelectivityEstimation() {
    std::cout << "Running testSelectivityEstimation..." << std::endl;

    auto& stats = optimizer::Statistics::getInstance();

    // 测试 selectivity 估算
    double selectivity = stats.estimateSelectivity("test_stats_db", "users", "age");
    assert(selectivity > 0 && selectivity <= 1.0 && "Selectivity should be between 0 and 1");

    std::cout << "  PASSED: selectivity = " << selectivity << std::endl;
}

void testRowCountEstimation() {
    std::cout << "Running testRowCountEstimation..." << std::endl;

    auto& stats = optimizer::Statistics::getInstance();

    // 测试行数估算
    int64_t estimatedRows = stats.estimateRowCount("test_stats_db", "users", 0.1);
    assert(estimatedRows >= 0 && "Estimated rows should be non-negative");

    std::cout << "  PASSED: estimatedRows = " << estimatedRows << std::endl;
}

void testStatisticsPersistence() {
    std::cout << "Running testStatisticsPersistence..." << std::endl;

    auto& stats = optimizer::Statistics::getInstance();

    // 清除缓存后加载
    stats.clearCache();
    auto result = stats.loadStatistics("test_stats_db", "users");
    assert(result.isSuccess() && "loadStatistics should succeed");

    auto tableStats = stats.getTableStatistics("test_stats_db", "users");
    assert(tableStats.isSuccess() && "Should load statistics");
    assert(tableStats.getValue()->rowCount > 0 && "Should have loaded row count");

    std::cout << "  PASSED: Loaded rowCount = " << tableStats.getValue()->rowCount << std::endl;
}

void cleanup() {
    auto& tableMgr = storage::TableManager::getInstance();
    tableMgr.dropDatabase("test_stats_db");
}

int main() {
    std::cout << "=== Statistics Module Tests ===" << std::endl;

    setupTestTable();

    testAnalyzeTable();
    testColumnStatistics();
    testSelectivityEstimation();
    testRowCountEstimation();
    testStatisticsPersistence();

    cleanup();

    std::cout << "\n=== All Statistics Tests PASSED ===" << std::endl;
    return 0;
}
