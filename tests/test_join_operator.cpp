#include "../src/executor/NestedLoopJoinOperator.h"
#include "../src/executor/TableScanOperator.h"
#include "../src/storage/TableManager.h"
#include "../src/parser/AST.h"
#include "../src/common/Type.h"
#include <cassert>
#include <iostream>

using namespace minisql;

void prepareTestTables() {
    auto& tableMgr = storage::TableManager::getInstance();

    // 创建数据库
    if (!tableMgr.databaseExists("test_join_db")) {
        tableMgr.createDatabase("test_join_db");
    }

    // 创建 users 表
    TableDef usersTable;
    usersTable.name = "users";
    usersTable.database = "test_join_db";

    ColumnDef userCol1;
    userCol1.name = "id";
    userCol1.type = DataType::INT;

    ColumnDef userCol2;
    userCol2.name = "name";
    userCol2.type = DataType::VARCHAR;
    userCol2.length = 50;

    ColumnDef userCol3;
    userCol3.name = "dept_id";
    userCol3.type = DataType::INT;

    usersTable.columns.push_back(userCol1);
    usersTable.columns.push_back(userCol2);
    usersTable.columns.push_back(userCol3);

    if (tableMgr.tableExists("test_join_db", "users").getValue()) {
        tableMgr.dropTable("test_join_db", "users");
    }
    tableMgr.createTable("test_join_db", usersTable);

    // 插��� users 数据
    tableMgr.insertRow("test_join_db", "users", {Value(1), Value("Alice"), Value(1)});
    tableMgr.insertRow("test_join_db", "users", {Value(2), Value("Bob"), Value(2)});
    tableMgr.insertRow("test_join_db", "users", {Value(3), Value("Charlie"), Value(1)});

    // 创建 departments 表
    TableDef deptTable;
    deptTable.name = "departments";
    deptTable.database = "test_join_db";

    ColumnDef deptCol1;
    deptCol1.name = "id";  // 改为 id 避免列名冲突
    deptCol1.type = DataType::INT;

    ColumnDef deptCol2;
    deptCol2.name = "dept_name";
    deptCol2.type = DataType::VARCHAR;
    deptCol2.length = 50;

    deptTable.columns.push_back(deptCol1);
    deptTable.columns.push_back(deptCol2);

    if (tableMgr.tableExists("test_join_db", "departments").getValue()) {
        tableMgr.dropTable("test_join_db", "departments");
    }
    tableMgr.createTable("test_join_db", deptTable);

    // 插入 departments 数据
    tableMgr.insertRow("test_join_db", "departments", {Value(1), Value("IT")});
    tableMgr.insertRow("test_join_db", "departments", {Value(2), Value("HR")});
}

void testInnerJoin() {
    std::cout << "Running testInnerJoin..." << std::endl;

    // 创建表扫描算子
    auto leftScan = std::make_shared<executor::TableScanOperator>("test_join_db", "users");
    auto rightScan = std::make_shared<executor::TableScanOperator>("test_join_db", "departments");

    // 创建连接条件: users.dept_id = departments.id
    auto leftCol = std::make_shared<parser::ColumnRef>("", "dept_id");
    auto rightCol = std::make_shared<parser::ColumnRef>("", "id");
    auto joinCond = std::make_shared<parser::BinaryExpr>(leftCol, "=", rightCol);

    // 创建连接算子
    auto joinOp = std::make_shared<executor::NestedLoopJoinOperator>(
        leftScan, rightScan, joinCond,
        executor::NestedLoopJoinOperator::JoinType::INNER);

    // 执行
    auto openResult = joinOp->open();
    assert(openResult.isSuccess());

    int count = 0;
    while (true) {
        auto nextResult = joinOp->next();
        assert(nextResult.isSuccess());

        auto rowOpt = nextResult.getValue();
        if (!rowOpt->has_value()) break;

        count++;
        const auto& row = rowOpt->value();
        // 验证连接结果：users.id, users.name, users.dept_id, departments.id, departments.dept_name
        assert(row.size() == 5);
        // 验证 dept_id 匹配（索引2是users.dept_id，索引3是departments.id）
        assert(row[2].getInt() == row[3].getInt());
    }

    assert(count == 3); // Alice->IT, Bob->HR, Charlie->IT

    joinOp->close();

    std::cout << "  Joined " << count << " rows" << std::endl;
    std::cout << "testInnerJoin: PASSED" << std::endl;
}

void testCrossJoin() {
    std::cout << "Running testCrossJoin..." << std::endl;

    // 创建表扫描算子
    auto leftScan = std::make_shared<executor::TableScanOperator>("test_join_db", "users");
    auto rightScan = std::make_shared<executor::TableScanOperator>("test_join_db", "departments");

    // 没有连接条件：笛卡尔积
    auto joinOp = std::make_shared<executor::NestedLoopJoinOperator>(
        leftScan, rightScan, nullptr,
        executor::NestedLoopJoinOperator::JoinType::INNER);

    // 执行
    auto openResult = joinOp->open();
    assert(openResult.isSuccess());

    int count = 0;
    while (true) {
        auto nextResult = joinOp->next();
        assert(nextResult.isSuccess());

        auto rowOpt = nextResult.getValue();
        if (!rowOpt->has_value()) break;

        count++;
        const auto& row = rowOpt->value();
        assert(row.size() == 5);
    }

    assert(count == 6); // 3 users × 2 departments

    joinOp->close();

    std::cout << "  Cartesian product: " << count << " rows" << std::endl;
    std::cout << "testCrossJoin: PASSED" << std::endl;
}

int main() {
    std::cout << "=== NestedLoopJoinOperator Tests ===" << std::endl;

    prepareTestTables();

    testInnerJoin();
    testCrossJoin();

    std::cout << "\n=== All tests PASSED ===" << std::endl;
    return 0;
}
