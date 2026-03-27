#include "../src/executor/TableScanOperator.h"
#include "../src/storage/TableManager.h"
#include "../src/storage/FileIO.h"
#include "../src/common/Type.h"
#include <cassert>
#include <iostream>

using namespace minisql;

void testFullTableScan() {
    std::cout << "Running testFullTableScan..." << std::endl;

    // 创建测试数据库
    auto& tableMgr = storage::TableManager::getInstance();
    if (!tableMgr.databaseExists("test_scan_db")) {
        auto dbResult = tableMgr.createDatabase("test_scan_db");
        assert(dbResult.isSuccess());
    }

    // 创建测试表（只有2列，避免自增ID问题）
    TableDef tableDef;
    tableDef.name = "users";
    tableDef.database = "test_scan_db";

    ColumnDef col1;
    col1.name = "name";
    col1.type = DataType::VARCHAR;
    col1.length = 50;
    col1.notNull = true;

    ColumnDef col2;
    col2.name = "age";
    col2.type = DataType::INT;

    tableDef.columns.push_back(col1);
    tableDef.columns.push_back(col2);

    // 删除已存在的表
    if (tableMgr.tableExists("test_scan_db", "users").getValue()) {
        tableMgr.dropTable("test_scan_db", "users");
    }

    auto createResult = tableMgr.createTable("test_scan_db", tableDef);
    assert(createResult.isSuccess());

    // 插入测试数据（不使用逗号）
    Row row1 = {Value("Alice"), Value(25)};
    Row row2 = {Value("Bob"), Value(30)};
    Row row3 = {Value("David"), Value(35)};

    tableMgr.insertRow("test_scan_db", "users", row1);
    tableMgr.insertRow("test_scan_db", "users", row2);
    tableMgr.insertRow("test_scan_db", "users", row3);

    // 调试：读取 CSV 文件内容
    std::string dataPath = "E:/pro/SQL_Management_System/build/data/test_scan_db/users.data";
    std::cout << "  CSV file content:" << std::endl;
    std::cout << minisql::storage::FileIO::readFromFile(dataPath) << std::endl;

    // 创建扫描算子
    auto scanOp = std::make_shared<executor::TableScanOperator>("test_scan_db", "users");

    // 执行扫描
    auto openResult = scanOp->open();
    assert(openResult.isSuccess());

    int rowCount = 0;
    while (true) {
        auto nextResult = scanOp->next();
        assert(nextResult.isSuccess());

        auto rowOpt = nextResult.getValue();
        if (!rowOpt->has_value()) break; // 没有更多行

        rowCount++;
        const auto& row = rowOpt->value();
        assert(row.size() == 2); // name 和 age

        // 验证列名
        auto columnNames = scanOp->getColumnNames();
        assert(columnNames.size() == 2);
    }

    assert(rowCount == 3);

    scanOp->close();

    std::cout << "testFullTableScan: PASSED" << std::endl;
}

void testScanWithColumnTypes() {
    std::cout << "Running testScanWithColumnTypes..." << std::endl;

    auto& tableMgr = storage::TableManager::getInstance();

    // 确保表存在
    if (!tableMgr.tableExists("test_scan_db", "users").getValue()) {
        std::cout << "  Skipping - table does not exist" << std::endl;
        return;
    }

    // 创建扫描算子
    auto scanOp = std::make_shared<executor::TableScanOperator>("test_scan_db", "users");

    auto openResult = scanOp->open();
    assert(openResult.isSuccess());

    // 验证列类型
    auto columnTypes = scanOp->getColumnTypes();
    assert(columnTypes.size() == 2);
    assert(columnTypes[0] == DataType::VARCHAR);  // name
    assert(columnTypes[1] == DataType::INT);  // age

    scanOp->close();

    std::cout << "testScanWithColumnTypes: PASSED" << std::endl;
}

int main() {
    std::cout << "=== TableScanOperator Tests ===" << std::endl;

    testFullTableScan();
    testScanWithColumnTypes();

    std::cout << "\n=== All tests PASSED ===" << std::endl;
    return 0;
}
