#include "../src/executor/FilterOperator.h"
#include "../src/executor/TableScanOperator.h"
#include "../src/storage/TableManager.h"
#include "../src/parser/AST.h"
#include "../src/common/Type.h"
#include <cassert>
#include <iostream>

using namespace minisql;

void prepareTestTable() {
    auto& tableMgr = storage::TableManager::getInstance();

    // 创建数据库
    if (!tableMgr.databaseExists("test_filter_db")) {
        tableMgr.createDatabase("test_filter_db");
    }

    // 创建表
    TableDef tableDef;
    tableDef.name = "employees";
    tableDef.database = "test_filter_db";

    ColumnDef col1;
    col1.name = "name";
    col1.type = DataType::VARCHAR;
    col1.length = 50;

    ColumnDef col2;
    col2.name = "salary";
    col2.type = DataType::INT;

    ColumnDef col3;
    col3.name = "department";
    col3.type = DataType::VARCHAR;
    col3.length = 50;

    tableDef.columns.push_back(col1);
    tableDef.columns.push_back(col2);
    tableDef.columns.push_back(col3);

    // 删除已存在的表
    if (tableMgr.tableExists("test_filter_db", "employees").getValue()) {
        tableMgr.dropTable("test_filter_db", "employees");
    }

    tableMgr.createTable("test_filter_db", tableDef);

    // 插入测试数据
    tableMgr.insertRow("test_filter_db", "employees", {Value("Alice"), Value(50000), Value("IT")});
    tableMgr.insertRow("test_filter_db", "employees", {Value("Bob"), Value(60000), Value("IT")});
    tableMgr.insertRow("test_filter_db", "employees", {Value("Charlie"), Value(55000), Value("HR")});
    tableMgr.insertRow("test_filter_db", "employees", {Value("David"), Value(45000), Value("IT")});
    tableMgr.insertRow("test_filter_db", "employees", {Value("Eve"), Value(70000), Value("HR")});
}

void testSimpleFilter() {
    std::cout << "Running testSimpleFilter..." << std::endl;

    // 创建表扫描算子
    auto scanOp = std::make_shared<executor::TableScanOperator>("test_filter_db", "employees");

    // 创建过滤条件: salary > 50000
    auto salaryCol = std::make_shared<parser::ColumnRef>("", "salary");
    auto value = std::make_shared<parser::LiteralExpr>(Value(50000));
    auto filterExpr = std::make_shared<parser::BinaryExpr>(salaryCol, ">", value);

    // 创建过滤算子
    auto filterOp = std::make_shared<executor::FilterOperator>(scanOp, filterExpr);

    // 执行
    auto openResult = filterOp->open();
    assert(openResult.isSuccess());

    int count = 0;
    while (true) {
        auto nextResult = filterOp->next();
        assert(nextResult.isSuccess());

        auto rowOpt = nextResult.getValue();
        if (!rowOpt->has_value()) break;

        count++;
        const auto& row = rowOpt->value();
        // 验证 salary > 50000
        assert(row[1].getInt() > 50000);
    }

    assert(count == 3); // Bob(60000), Charlie(55000), Eve(70000)

    filterOp->close();

    std::cout << "  Filtered " << count << " rows" << std::endl;
    std::cout << "testSimpleFilter: PASSED" << std::endl;
}

void testComplexFilter() {
    std::cout << "Running testComplexFilter..." << std::endl;

    // 创建表扫描算子
    auto scanOp = std::make_shared<executor::TableScanOperator>("test_filter_db", "employees");

    // 创建复合过滤条件: department = 'IT' AND salary >= 50000
    auto deptCol = std::make_shared<parser::ColumnRef>("", "department");
    auto itValue = std::make_shared<parser::LiteralExpr>(Value("IT"));
    auto deptCondition = std::make_shared<parser::BinaryExpr>(deptCol, "=", itValue);

    auto salaryCol = std::make_shared<parser::ColumnRef>("", "salary");
    auto salaryValue = std::make_shared<parser::LiteralExpr>(Value(50000));
    auto salaryCondition = std::make_shared<parser::BinaryExpr>(salaryCol, ">=", salaryValue);

    auto filterExpr = std::make_shared<parser::BinaryExpr>(deptCondition, "AND", salaryCondition);

    // 创建过滤算子
    auto filterOp = std::make_shared<executor::FilterOperator>(scanOp, filterExpr);

    // 执行
    auto openResult = filterOp->open();
    assert(openResult.isSuccess());

    int count = 0;
    while (true) {
        auto nextResult = filterOp->next();
        assert(nextResult.isSuccess());

        auto rowOpt = nextResult.getValue();
        if (!rowOpt->has_value()) break;

        count++;
        const auto& row = rowOpt->value();
        // 验证条件
        assert(row[2].getString() == "IT");
        assert(row[1].getInt() >= 50000);
    }

    assert(count == 2); // Alice(50000,IT), Bob(60000,IT)

    filterOp->close();

    std::cout << "  Filtered " << count << " rows" << std::endl;
    std::cout << "testComplexFilter: PASSED" << std::endl;
}

void testNoMatchFilter() {
    std::cout << "Running testNoMatchFilter..." << std::endl;

    // 创建表扫描算子
    auto scanOp = std::make_shared<executor::TableScanOperator>("test_filter_db", "employees");

    // 创建不可能满足的条件: salary > 100000
    auto salaryCol = std::make_shared<parser::ColumnRef>("", "salary");
    auto value = std::make_shared<parser::LiteralExpr>(Value(100000));
    auto filterExpr = std::make_shared<parser::BinaryExpr>(salaryCol, ">", value);

    // 创建过滤算子
    auto filterOp = std::make_shared<executor::FilterOperator>(scanOp, filterExpr);

    // 执行
    auto openResult = filterOp->open();
    assert(openResult.isSuccess());

    int count = 0;
    while (true) {
        auto nextResult = filterOp->next();
        assert(nextResult.isSuccess());

        auto rowOpt = nextResult.getValue();
        if (!rowOpt->has_value()) break;

        count++;
    }

    assert(count == 0); // 没有匹配的行

    filterOp->close();

    std::cout << "  No rows matched (as expected)" << std::endl;
    std::cout << "testNoMatchFilter: PASSED" << std::endl;
}

int main() {
    std::cout << "=== FilterOperator Tests ===" << std::endl;

    prepareTestTable();

    testSimpleFilter();
    testComplexFilter();
    testNoMatchFilter();

    std::cout << "\n=== All tests PASSED ===" << std::endl;
    return 0;
}
