#include "../src/optimizer/CostOptimizer.h"
#include "../src/optimizer/Statistics.h"
#include "../src/storage/TableManager.h"
#include "../src/common/Type.h"
#include <cassert>
#include <iostream>

using namespace minisql;

void setupTestTable() {
    auto& tableMgr = storage::TableManager::getInstance();

    if (tableMgr.databaseExists("test_cost_db")) {
        tableMgr.dropDatabase("test_cost_db");
    }
    tableMgr.createDatabase("test_cost_db");

    TableDef tableDef;
    tableDef.name = "orders";
    tableDef.database = "test_cost_db";

    ColumnDef idCol;
    idCol.name = "id";
    idCol.type = DataType::INT;
    idCol.primaryKey = true;

    ColumnDef amountCol;
    amountCol.name = "amount";
    amountCol.type = DataType::DOUBLE;

    ColumnDef statusCol;
    statusCol.name = "status";
    statusCol.type = DataType::INT;

    tableDef.columns.push_back(idCol);
    tableDef.columns.push_back(amountCol);
    tableDef.columns.push_back(statusCol);

    tableMgr.createTable("test_cost_db", tableDef);

    for (int i = 1; i <= 100; ++i) {
        double amount = 10.0 + (i * 1.5);
        int status = (i % 3) + 1;
        tableMgr.insertRow("test_cost_db", "orders",
            {Value(i), Value(amount), Value(status)});
    }
}

void testCostEstimateCreation() {
    std::cout << "Running testCostEstimateCreation..." << std::endl;

    optimizer::CostEstimate cost(10.0, 5.0, 100);
    assert(cost.cpuCost == 10.0);
    assert(cost.ioCost == 5.0);
    assert(cost.estimatedRows == 100);
    assert(cost.totalCost == 15.0);

    std::cout << "  PASSED: CostEstimate(total=" << cost.totalCost << ")" << std::endl;
}

void testScanCostEstimation() {
    std::cout << "Running testScanCostEstimation..." << std::endl;

    // 先分析表以收集统计信息
    optimizer::Statistics::getInstance().analyzeTable("test_cost_db", "orders");

    auto cost = optimizer::CostOptimizer::estimateScanCost("test_cost_db", "orders", nullptr);
    assert(cost.estimatedRows >= 0);
    assert(cost.ioCost >= 0);

    std::cout << "  PASSED: estimatedRows=" << cost.estimatedRows
              << ", ioCost=" << cost.ioCost << std::endl;
}

void testCostModelConstants() {
    std::cout << "Running testCostModelConstants..." << std::endl;

    using namespace optimizer;
    assert(CostModel::CPU_TABLE_SCAN > 0);
    assert(CostModel::CPU_INDEX_SCAN > 0);
    assert(CostModel::CPU_NESTED_LOOP > 0);
    assert(CostModel::CPU_FILTER > 0);
    assert(CostModel::CPU_SORT > 0);
    assert(CostModel::PAGE_SIZE > 0);

    std::cout << "  PASSED: All cost model constants are positive" << std::endl;
}

void testFilterCostEstimation() {
    std::cout << "Running testFilterCostEstimation..." << std::endl;

    auto cost = optimizer::CostOptimizer::estimateFilterCost("test_cost_db", nullptr, nullptr);
    assert(cost.estimatedRows >= 0);

    std::cout << "  PASSED: Filter cost estimation completed" << std::endl;
}

void testSortCostEstimation() {
    std::cout << "Running testSortCostEstimation..." << std::endl;

    std::vector<parser::OrderByItem> orderBy;
    auto cost = optimizer::CostOptimizer::estimateSortCost("test_cost_db", nullptr, orderBy);
    assert(cost.cpuCost >= 0);

    std::cout << "  PASSED: Sort cost estimation completed" << std::endl;
}

void testAggregateCostEstimation() {
    std::cout << "Running testAggregateCostEstimation..." << std::endl;

    std::vector<parser::ExprPtr> groupBy;
    auto cost = optimizer::CostOptimizer::estimateAggregateCost("test_cost_db", nullptr, groupBy);
    assert(cost.estimatedRows >= 0);

    std::cout << "  PASSED: Aggregate cost estimation completed" << std::endl;
}

void cleanup() {
    auto& tableMgr = storage::TableManager::getInstance();
    tableMgr.dropDatabase("test_cost_db");
}

int main() {
    std::cout << "=== CostOptimizer Module Tests ===" << std::endl;

    setupTestTable();

    testCostEstimateCreation();
    testScanCostEstimation();
    testCostModelConstants();
    testFilterCostEstimation();
    testSortCostEstimation();
    testAggregateCostEstimation();

    cleanup();

    std::cout << "\n=== All CostOptimizer Tests PASSED ===" << std::endl;
    return 0;
}
