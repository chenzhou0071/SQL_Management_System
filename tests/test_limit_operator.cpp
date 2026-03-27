#include "../src/executor/LimitOperator.h"
#include "../src/executor/TableScanOperator.h"
#include "../src/storage/TableManager.h"
#include "../src/common/Type.h"
#include <cassert>
#include <iostream>

using namespace minisql;

void prepareTestTable() {
    auto& tableMgr = storage::TableManager::getInstance();

    if (!tableMgr.databaseExists("test_limit_db")) {
        tableMgr.createDatabase("test_limit_db");
    }

    TableDef tableDef;
    tableDef.name = "numbers";
    tableDef.database = "test_limit_db";

    ColumnDef col1;
    col1.name = "value";
    col1.type = DataType::INT;

    tableDef.columns.push_back(col1);

    if (tableMgr.tableExists("test_limit_db", "numbers").getValue()) {
        tableMgr.dropTable("test_limit_db", "numbers");
    }
    tableMgr.createTable("test_limit_db", tableDef);

    // 插入 10 行数据
    for (int i = 1; i <= 10; i++) {
        tableMgr.insertRow("test_limit_db", "numbers", {Value(i)});
    }
}

void testLimitOnly() {
    std::cout << "Running testLimitOnly..." << std::endl;

    auto scanOp = std::make_shared<executor::TableScanOperator>("test_limit_db", "numbers");
    auto limitOp = std::make_shared<executor::LimitOperator>(scanOp, 5, 0);

    auto openResult = limitOp->open();
    assert(openResult.isSuccess());

    int count = 0;
    int sum = 0;
    while (true) {
        auto nextResult = limitOp->next();
        assert(nextResult.isSuccess());

        auto rowOpt = nextResult.getValue();
        if (!rowOpt->has_value()) break;

        count++;
        sum += rowOpt->value()[0].getInt();
    }

    assert(count == 5);
    assert(sum == 15); // 1+2+3+4+5 = 15

    limitOp->close();

    std::cout << "  Limited to " << count << " rows" << std::endl;
    std::cout << "testLimitOnly: PASSED" << std::endl;
}

void testLimitWithOffset() {
    std::cout << "Running testLimitWithOffset..." << std::endl;

    auto scanOp = std::make_shared<executor::TableScanOperator>("test_limit_db", "numbers");
    auto limitOp = std::make_shared<executor::LimitOperator>(scanOp, 3, 5);

    auto openResult = limitOp->open();
    assert(openResult.isSuccess());

    int count = 0;
    int sum = 0;
    while (true) {
        auto nextResult = limitOp->next();
        assert(nextResult.isSuccess());

        auto rowOpt = nextResult.getValue();
        if (!rowOpt->has_value()) break;

        count++;
        sum += rowOpt->value()[0].getInt();
    }

    assert(count == 3);
    assert(sum == 21); // 6+7+8 = 21

    limitOp->close();

    std::cout << "  Skipped 5, limited to " << count << " rows" << std::endl;
    std::cout << "testLimitWithOffset: PASSED" << std::endl;
}

void testOffsetExceedsData() {
    std::cout << "Running testOffsetExceedsData..." << std::endl;

    auto scanOp = std::make_shared<executor::TableScanOperator>("test_limit_db", "numbers");
    auto limitOp = std::make_shared<executor::LimitOperator>(scanOp, 5, 20);

    auto openResult = limitOp->open();
    assert(openResult.isSuccess());

    int count = 0;
    while (true) {
        auto nextResult = limitOp->next();
        assert(nextResult.isSuccess());

        auto rowOpt = nextResult.getValue();
        if (!rowOpt->has_value()) break;

        count++;
    }

    assert(count == 0); // OFFSET 超过数据量

    limitOp->close();

    std::cout << "  No rows (offset exceeds data)" << std::endl;
    std::cout << "testOffsetExceedsData: PASSED" << std::endl;
}

void testZeroLimit() {
    std::cout << "Running testZeroLimit..." << std::endl;

    auto scanOp = std::make_shared<executor::TableScanOperator>("test_limit_db", "numbers");
    auto limitOp = std::make_shared<executor::LimitOperator>(scanOp, 0, 0);

    auto openResult = limitOp->open();
    assert(openResult.isSuccess());

    int count = 0;
    while (true) {
        auto nextResult = limitOp->next();
        assert(nextResult.isSuccess());

        auto rowOpt = nextResult.getValue();
        if (!rowOpt->has_value()) break;

        count++;
    }

    assert(count == 0); // LIMIT 0

    limitOp->close();

    std::cout << "  No rows (LIMIT 0)" << std::endl;
    std::cout << "testZeroLimit: PASSED" << std::endl;
}

int main() {
    std::cout << "=== LimitOperator Tests ===" << std::endl;

    prepareTestTable();

    testLimitOnly();
    testLimitWithOffset();
    testOffsetExceedsData();
    testZeroLimit();

    std::cout << "\n=== All tests PASSED ===" << std::endl;
    return 0;
}
