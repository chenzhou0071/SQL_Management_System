#include "../src/executor/AggregateOperator.h"
#include "../src/executor/TableScanOperator.h"
#include "../src/storage/TableManager.h"
#include "../src/parser/AST.h"
#include "../src/common/Type.h"
#include <cassert>
#include <iostream>
#include <cmath>

using namespace minisql;

void prepareTestTable() {
    auto& tableMgr = storage::TableManager::getInstance();

    if (!tableMgr.databaseExists("test_aggregate_db")) {
        tableMgr.createDatabase("test_aggregate_db");
    }

    TableDef tableDef;
    tableDef.name = "sales";
    tableDef.database = "test_aggregate_db";

    ColumnDef col1;
    col1.name = "department";
    col1.type = DataType::VARCHAR;
    col1.length = 50;

    ColumnDef col2;
    col2.name = "amount";
    col2.type = DataType::INT;

    tableDef.columns.push_back(col1);
    tableDef.columns.push_back(col2);

    if (tableMgr.tableExists("test_aggregate_db", "sales").getValue()) {
        tableMgr.dropTable("test_aggregate_db", "sales");
    }
    tableMgr.createTable("test_aggregate_db", tableDef);

    // 插入测试数据
    tableMgr.insertRow("test_aggregate_db", "sales", {Value("IT"), Value(100)});
    tableMgr.insertRow("test_aggregate_db", "sales", {Value("IT"), Value(200)});
    tableMgr.insertRow("test_aggregate_db", "sales", {Value("IT"), Value(150)});
    tableMgr.insertRow("test_aggregate_db", "sales", {Value("HR"), Value(80)});
    tableMgr.insertRow("test_aggregate_db", "sales", {Value("HR"), Value(120)});
}

void testCountAggregate() {
    std::cout << "Running testCountAggregate..." << std::endl;

    auto scanOp = std::make_shared<executor::TableScanOperator>("test_aggregate_db", "sales");

    // COUNT(*) 没有 GROUP BY
    std::vector<parser::ExprPtr> groupBy;
    std::vector<executor::AggregateFunc> aggregates;
    aggregates.push_back(executor::AggregateFunc(
        "COUNT",
        std::make_shared<parser::LiteralExpr>(Value(1))  // COUNT(*)
    ));

    auto aggOp = std::make_shared<executor::AggregateOperator>(scanOp, groupBy, aggregates);

    auto openResult = aggOp->open();
    assert(openResult.isSuccess());

    auto nextResult = aggOp->next();
    assert(nextResult.isSuccess());

    auto rowOpt = nextResult.getValue();
    assert(rowOpt->has_value());

    const auto& row = rowOpt->value();
    assert(row.size() == 1);
    assert(row[0].getInt() == 5); // 总共 5 行

    aggOp->close();

    std::cout << "  COUNT(*) = " << row[0].getInt() << std::endl;
    std::cout << "testCountAggregate: PASSED" << std::endl;
}

void testGroupByCount() {
    std::cout << "Running testGroupByCount..." << std::endl;

    auto scanOp = std::make_shared<executor::TableScanOperator>("test_aggregate_db", "sales");

    // COUNT(*) GROUP BY department
    std::vector<parser::ExprPtr> groupBy;
    groupBy.push_back(std::make_shared<parser::ColumnRef>("", "department"));

    std::vector<executor::AggregateFunc> aggregates;
    aggregates.push_back(executor::AggregateFunc(
        "COUNT",
        std::make_shared<parser::LiteralExpr>(Value(1))
    ));

    auto aggOp = std::make_shared<executor::AggregateOperator>(scanOp, groupBy, aggregates);

    auto openResult = aggOp->open();
    assert(openResult.isSuccess());

    int groupCount = 0;
    while (true) {
        auto nextResult = aggOp->next();
        assert(nextResult.isSuccess());

        auto rowOpt = nextResult.getValue();
        if (!rowOpt->has_value()) break;

        groupCount++;
        const auto& row = rowOpt->value();
        assert(row.size() == 2);
        // department, COUNT
        std::cout << "    " << row[0].getString() << ": " << row[1].getInt() << " rows" << std::endl;
    }

    assert(groupCount == 2); // IT 和 HR

    aggOp->close();

    std::cout << "testGroupByCount: PASSED" << std::endl;
}

void testSumAvgAggregate() {
    std::cout << "Running testSumAvgAggregate..." << std::endl;

    auto scanOp = std::make_shared<executor::TableScanOperator>("test_aggregate_db", "sales");

    // 先只测试 SUM
    std::vector<parser::ExprPtr> groupBy;
    groupBy.push_back(std::make_shared<parser::ColumnRef>("", "department"));

    std::vector<executor::AggregateFunc> aggregates;
    aggregates.push_back(executor::AggregateFunc(
        "SUM",
        std::make_shared<parser::ColumnRef>("", "amount")
    ));
    aggregates.push_back(executor::AggregateFunc(
        "AVG",
        std::make_shared<parser::ColumnRef>("", "amount")
    ));

    auto aggOp = std::make_shared<executor::AggregateOperator>(scanOp, groupBy, aggregates);

    auto openResult = aggOp->open();
    assert(openResult.isSuccess());

    while (true) {
        auto nextResult = aggOp->next();
        assert(nextResult.isSuccess());

        auto rowOpt = nextResult.getValue();
        if (!rowOpt->has_value()) break;

        const auto& row = rowOpt->value();
        assert(row.size() == 3);
        // department, SUM, AVG
        std::string dept = row[0].getString();
        double sum = row[1].getDouble();
        double avg = row[2].getDouble();

        std::cout << "    " << dept << ": SUM=" << sum << ", AVG=" << avg << std::endl;

        if (dept == "IT") {
            assert(std::abs(sum - 450.0) < 0.01);  // 100+200+150 = 450
            assert(std::abs(avg - 150.0) < 0.01);  // 450/3 = 150
        } else if (dept == "HR") {
            assert(std::abs(sum - 200.0) < 0.01);  // 80+120 = 200
            assert(std::abs(avg - 100.0) < 0.01);  // 200/2 = 100
        }
    }

    aggOp->close();

    std::cout << "testSumAvgAggregate: PASSED" << std::endl;
}

void testMinMaxAggregate() {
    std::cout << "Running testMinMaxAggregate..." << std::endl;

    auto scanOp = std::make_shared<executor::TableScanOperator>("test_aggregate_db", "sales");

    // MIN(amount), MAX(amount) GROUP BY department
    std::vector<parser::ExprPtr> groupBy;
    groupBy.push_back(std::make_shared<parser::ColumnRef>("", "department"));

    std::vector<executor::AggregateFunc> aggregates;
    aggregates.push_back(executor::AggregateFunc(
        "MIN",
        std::make_shared<parser::ColumnRef>("", "amount")
    ));
    aggregates.push_back(executor::AggregateFunc(
        "MAX",
        std::make_shared<parser::ColumnRef>("", "amount")
    ));

    auto aggOp = std::make_shared<executor::AggregateOperator>(scanOp, groupBy, aggregates);

    auto openResult = aggOp->open();
    assert(openResult.isSuccess());

    while (true) {
        auto nextResult = aggOp->next();
        assert(nextResult.isSuccess());

        auto rowOpt = nextResult.getValue();
        if (!rowOpt->has_value()) break;

        const auto& row = rowOpt->value();
        assert(row.size() == 3);
        // department, MIN, MAX
        std::string dept = row[0].getString();
        int minVal = row[1].getInt();
        int maxVal = row[2].getInt();

        if (dept == "IT") {
            assert(minVal == 100);
            assert(maxVal == 200);
        } else if (dept == "HR") {
            assert(minVal == 80);
            assert(maxVal == 120);
        }

        std::cout << "    " << dept << ": MIN=" << minVal << ", MAX=" << maxVal << std::endl;
    }

    aggOp->close();

    std::cout << "testMinMaxAggregate: PASSED" << std::endl;
}

int main() {
    std::cout << "=== AggregateOperator Tests ===" << std::endl;

    prepareTestTable();

    testCountAggregate();
    testGroupByCount();
    testSumAvgAggregate();
    testMinMaxAggregate();

    std::cout << "\n=== All tests PASSED ===" << std::endl;
    return 0;
}
