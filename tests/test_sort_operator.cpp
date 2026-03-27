#include "../src/executor/SortOperator.h"
#include "../src/executor/TableScanOperator.h"
#include "../src/storage/TableManager.h"
#include "../src/parser/AST.h"
#include "../src/common/Type.h"
#include <cassert>
#include <iostream>

using namespace minisql;

void prepareTestTable() {
    auto& tableMgr = storage::TableManager::getInstance();

    if (!tableMgr.databaseExists("test_sort_db")) {
        tableMgr.createDatabase("test_sort_db");
    }

    TableDef tableDef;
    tableDef.name = "employees";
    tableDef.database = "test_sort_db";

    ColumnDef col1;
    col1.name = "name";
    col1.type = DataType::VARCHAR;
    col1.length = 50;

    ColumnDef col2;
    col2.name = "salary";
    col2.type = DataType::INT;

    ColumnDef col3;
    col3.name = "age";
    col3.type = DataType::INT;

    tableDef.columns.push_back(col1);
    tableDef.columns.push_back(col2);
    tableDef.columns.push_back(col3);

    if (tableMgr.tableExists("test_sort_db", "employees").getValue()) {
        tableMgr.dropTable("test_sort_db", "employees");
    }
    tableMgr.createTable("test_sort_db", tableDef);

    // 插入测试数据（无序）
    tableMgr.insertRow("test_sort_db", "employees", {Value("Charlie"), Value(50000), Value(30)});
    tableMgr.insertRow("test_sort_db", "employees", {Value("Alice"), Value(60000), Value(25)});
    tableMgr.insertRow("test_sort_db", "employees", {Value("Bob"), Value(45000), Value(35)});
    tableMgr.insertRow("test_sort_db", "employees", {Value("David"), Value(70000), Value(28)});
}

void testSingleColumnAsc() {
    std::cout << "Running testSingleColumnAsc..." << std::endl;

    auto scanOp = std::make_shared<executor::TableScanOperator>("test_sort_db", "employees");

    // 按 salary ASC 排序
    std::vector<executor::SortItem> orderBy;
    orderBy.push_back(executor::SortItem(
        std::make_shared<parser::ColumnRef>("", "salary"),
        true
    ));

    auto sortOp = std::make_shared<executor::SortOperator>(scanOp, orderBy);

    auto openResult = sortOp->open();
    assert(openResult.isSuccess());

    std::vector<int> salaries;
    while (true) {
        auto nextResult = sortOp->next();
        assert(nextResult.isSuccess());

        auto rowOpt = nextResult.getValue();
        if (!rowOpt->has_value()) break;

        salaries.push_back(rowOpt->value()[1].getInt());
    }

    // 验证升序排列
    assert(salaries.size() == 4);
    assert(salaries[0] == 45000);
    assert(salaries[1] == 50000);
    assert(salaries[2] == 60000);
    assert(salaries[3] == 70000);

    sortOp->close();

    std::cout << "  Sorted by salary ASC" << std::endl;
    std::cout << "testSingleColumnAsc: PASSED" << std::endl;
}

void testSingleColumnDesc() {
    std::cout << "Running testSingleColumnDesc..." << std::endl;

    auto scanOp = std::make_shared<executor::TableScanOperator>("test_sort_db", "employees");

    // 按 salary DESC 排序
    std::vector<executor::SortItem> orderBy;
    orderBy.push_back(executor::SortItem(
        std::make_shared<parser::ColumnRef>("", "salary"),
        false
    ));

    auto sortOp = std::make_shared<executor::SortOperator>(scanOp, orderBy);

    auto openResult = sortOp->open();
    assert(openResult.isSuccess());

    std::vector<int> salaries;
    while (true) {
        auto nextResult = sortOp->next();
        assert(nextResult.isSuccess());

        auto rowOpt = nextResult.getValue();
        if (!rowOpt->has_value()) break;

        salaries.push_back(rowOpt->value()[1].getInt());
    }

    // 验证降序排列
    assert(salaries.size() == 4);
    assert(salaries[0] == 70000);
    assert(salaries[1] == 60000);
    assert(salaries[2] == 50000);
    assert(salaries[3] == 45000);

    sortOp->close();

    std::cout << "  Sorted by salary DESC" << std::endl;
    std::cout << "testSingleColumnDesc: PASSED" << std::endl;
}

void testMultiColumnSort() {
    std::cout << "Running testMultiColumnSort..." << std::endl;

    auto scanOp = std::make_shared<executor::TableScanOperator>("test_sort_db", "employees");

    // 按 age ASC, salary DESC 排序
    std::vector<executor::SortItem> orderBy;
    orderBy.push_back(executor::SortItem(
        std::make_shared<parser::ColumnRef>("", "age"),
        true
    ));
    orderBy.push_back(executor::SortItem(
        std::make_shared<parser::ColumnRef>("", "salary"),
        false
    ));

    auto sortOp = std::make_shared<executor::SortOperator>(scanOp, orderBy);

    auto openResult = sortOp->open();
    assert(openResult.isSuccess());

    std::vector<std::pair<int, int>> ageSalaryPairs;
    while (true) {
        auto nextResult = sortOp->next();
        assert(nextResult.isSuccess());

        auto rowOpt = nextResult.getValue();
        if (!rowOpt->has_value()) break;

        const auto& row = rowOpt->value();
        ageSalaryPairs.push_back({row[2].getInt(), row[1].getInt()});
    }

    // 验证排序结果
    assert(ageSalaryPairs.size() == 4);
    // age ASC: 25, 28, 30, 35
    assert(ageSalaryPairs[0].first == 25);
    assert(ageSalaryPairs[1].first == 28);
    assert(ageSalaryPairs[2].first == 30);
    assert(ageSalaryPairs[3].first == 35);

    sortOp->close();

    std::cout << "  Sorted by age ASC, salary DESC" << std::endl;
    std::cout << "testMultiColumnSort: PASSED" << std::endl;
}

int main() {
    std::cout << "=== SortOperator Tests ===" << std::endl;

    prepareTestTable();

    testSingleColumnAsc();
    testSingleColumnDesc();
    testMultiColumnSort();

    std::cout << "\n=== All tests PASSED ===" << std::endl;
    return 0;
}
