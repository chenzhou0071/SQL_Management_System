#include "../src/executor/ProjectOperator.h"
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
    if (!tableMgr.databaseExists("test_project_db")) {
        tableMgr.createDatabase("test_project_db");
    }

    // 创建表
    TableDef tableDef;
    tableDef.name = "products";
    tableDef.database = "test_project_db";

    ColumnDef col1;
    col1.name = "name";
    col1.type = DataType::VARCHAR;
    col1.length = 50;

    ColumnDef col2;
    col2.name = "price";
    col2.type = DataType::INT;

    ColumnDef col3;
    col3.name = "quantity";
    col3.type = DataType::INT;

    tableDef.columns.push_back(col1);
    tableDef.columns.push_back(col2);
    tableDef.columns.push_back(col3);

    // 删除已存在的表
    if (tableMgr.tableExists("test_project_db", "products").getValue()) {
        tableMgr.dropTable("test_project_db", "products");
    }

    tableMgr.createTable("test_project_db", tableDef);

    // 插入测试数据
    tableMgr.insertRow("test_project_db", "products", {Value("Apple"), Value(10), Value(5)});
    tableMgr.insertRow("test_project_db", "products", {Value("Banana"), Value(5), Value(10)});
    tableMgr.insertRow("test_project_db", "products", {Value("Orange"), Value(8), Value(3)});
}

void testSimpleProjection() {
    std::cout << "Running testSimpleProjection..." << std::endl;

    // 创建表扫描算子
    auto scanOp = std::make_shared<executor::TableScanOperator>("test_project_db", "products");

    // 创建投影列表: name, price
    std::vector<parser::ExprPtr> projections;
    projections.push_back(std::make_shared<parser::ColumnRef>("", "name"));
    projections.push_back(std::make_shared<parser::ColumnRef>("", "price"));

    // 创建投影算子
    auto projectOp = std::make_shared<executor::ProjectOperator>(scanOp, projections);

    // 执行
    auto openResult = projectOp->open();
    assert(openResult.isSuccess());

    // 验证列名
    auto columnNames = projectOp->getColumnNames();
    assert(columnNames.size() == 2);
    assert(columnNames[0] == "name");
    assert(columnNames[1] == "price");

    // 验证列类型
    auto columnTypes = projectOp->getColumnTypes();
    assert(columnTypes.size() == 2);
    assert(columnTypes[0] == DataType::VARCHAR);
    assert(columnTypes[1] == DataType::INT);

    int count = 0;
    while (true) {
        auto nextResult = projectOp->next();
        assert(nextResult.isSuccess());

        auto rowOpt = nextResult.getValue();
        if (!rowOpt->has_value()) break;

        count++;
        const auto& row = rowOpt->value();
        assert(row.size() == 2);
    }

    assert(count == 3);

    projectOp->close();

    std::cout << "testSimpleProjection: PASSED" << std::endl;
}

void testExpressionProjection() {
    std::cout << "Running testExpressionProjection..." << std::endl;

    // 创建表扫描算子
    auto scanOp = std::make_shared<executor::TableScanOperator>("test_project_db", "products");

    // 创建投影列表: name, price * quantity
    std::vector<parser::ExprPtr> projections;
    projections.push_back(std::make_shared<parser::ColumnRef>("", "name"));

    auto priceCol = std::make_shared<parser::ColumnRef>("", "price");
    auto qtyCol = std::make_shared<parser::ColumnRef>("", "quantity");
    projections.push_back(std::make_shared<parser::BinaryExpr>(priceCol, "*", qtyCol));

    // 创建投影算子
    auto projectOp = std::make_shared<executor::ProjectOperator>(scanOp, projections);

    // 执行
    auto openResult = projectOp->open();
    assert(openResult.isSuccess());

    int count = 0;
    while (true) {
        auto nextResult = projectOp->next();
        assert(nextResult.isSuccess());

        auto rowOpt = nextResult.getValue();
        if (!rowOpt->has_value()) break;

        count++;
        const auto& row = rowOpt->value();
        assert(row.size() == 2);
        // 验证计算结果
        assert(row[1].getInt() >= 0);
    }

    assert(count == 3);

    projectOp->close();

    std::cout << "testExpressionProjection: PASSED" << std::endl;
}

void testFunctionProjection() {
    std::cout << "Running testFunctionProjection..." << std::endl;

    // 创建表扫描算子
    auto scanOp = std::make_shared<executor::TableScanOperator>("test_project_db", "products");

    // 创建投影列表: UPPER(name), price
    std::vector<parser::ExprPtr> projections;

    auto nameCol = std::make_shared<parser::ColumnRef>("", "name");
    std::vector<parser::ExprPtr> upperArgs = {nameCol};
    projections.push_back(std::make_shared<parser::FunctionCallExpr>("UPPER", upperArgs));

    projections.push_back(std::make_shared<parser::ColumnRef>("", "price"));

    // 创建投影算子
    auto projectOp = std::make_shared<executor::ProjectOperator>(scanOp, projections);

    // 执行
    auto openResult = projectOp->open();
    assert(openResult.isSuccess());

    int count = 0;
    while (true) {
        auto nextResult = projectOp->next();
        assert(nextResult.isSuccess());

        auto rowOpt = nextResult.getValue();
        if (!rowOpt->has_value()) break;

        count++;
        const auto& row = rowOpt->value();
        assert(row.size() == 2);
        // 验证 UPPER 函数
        std::string upperName = row[0].getString();
        for (char c : upperName) {
            assert(std::isupper(c) || std::isdigit(c));
        }
    }

    assert(count == 3);

    projectOp->close();

    std::cout << "testFunctionProjection: PASSED" << std::endl;
}

int main() {
    std::cout << "=== ProjectOperator Tests ===" << std::endl;

    prepareTestTable();

    testSimpleProjection();
    testExpressionProjection();
    testFunctionProjection();

    std::cout << "\n=== All tests PASSED ===" << std::endl;
    return 0;
}
