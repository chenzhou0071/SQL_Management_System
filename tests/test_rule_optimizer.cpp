#include "../src/optimizer/RuleOptimizer.h"
#include "../src/executor/TableScanOperator.h"
#include "../src/executor/FilterOperator.h"
#include "../src/storage/TableManager.h"
#include "../src/parser/AST.h"
#include "../src/common/Type.h"
#include <cassert>
#include <iostream>

using namespace minisql;

void setupTestTable() {
    auto& tableMgr = storage::TableManager::getInstance();

    if (tableMgr.databaseExists("test_rule_db")) {
        tableMgr.dropDatabase("test_rule_db");
    }
    tableMgr.createDatabase("test_rule_db");

    TableDef tableDef;
    tableDef.name = "users";
    tableDef.database = "test_rule_db";

    ColumnDef idCol;
    idCol.name = "id";
    idCol.type = DataType::INT;
    idCol.primaryKey = true;

    ColumnDef nameCol;
    nameCol.name = "name";
    nameCol.type = DataType::VARCHAR;
    nameCol.length = 50;

    ColumnDef ageCol;
    ageCol.name = "age";
    ageCol.type = DataType::INT;

    tableDef.columns.push_back(idCol);
    tableDef.columns.push_back(nameCol);
    tableDef.columns.push_back(ageCol);

    tableMgr.createTable("test_rule_db", tableDef);

    for (int i = 1; i <= 20; ++i) {
        std::string name = "User" + std::to_string(i);
        int age = 20 + (i % 40);
        tableMgr.insertRow("test_rule_db", "users",
            {Value(i), Value(name), Value(age)});
    }
}

void testOptimizerSingleton() {
    std::cout << "Running testOptimizerSingleton..." << std::endl;

    auto& instance1 = optimizer::RuleOptimizer::getInstance();
    auto& instance2 = optimizer::RuleOptimizer::getInstance();

    // 单例模式应该返回同一实例
    assert(&instance1 == &instance2);

    std::cout << "  PASSED: Singleton pattern works" << std::endl;
}

void testOptimizeNullPlan() {
    std::cout << "Running testOptimizeNullPlan..." << std::endl;

    auto result = optimizer::RuleOptimizer::optimize(nullptr);
    // 空计划应该返回空
    assert(!result.isError());

    std::cout << "  PASSED: Null plan optimization handled" << std::endl;
}

void testOptimizeSimplePlan() {
    std::cout << "Running testOptimizeSimplePlan..." << std::endl;

    // 创建简单的表扫描算子
    auto tableScan = std::make_shared<executor::TableScanOperator>("test_rule_db", "users");
    auto result = optimizer::RuleOptimizer::optimize(tableScan);

    assert(result.isSuccess());
    assert(result.getValue() != nullptr);

    std::cout << "  PASSED: Simple plan optimization completed" << std::endl;
}

void testAddCustomRule() {
    std::cout << "Running testAddCustomRule..." << std::endl;

    optimizer::OptimizerRule customRule;
    customRule.name = "custom_rule";
    customRule.priority = 0;  // 最高优先级
    customRule.apply = [](executor::OperatorPtr plan) -> executor::OperatorPtr {
        return plan;  // 不做任何修改
    };

    optimizer::RuleOptimizer::addRule(customRule);

    // 添加规则后应该仍然可以正常优化
    auto tableScan = std::make_shared<executor::TableScanOperator>("test_rule_db", "users");
    auto result = optimizer::RuleOptimizer::optimize(tableScan);

    assert(result.isSuccess());

    std::cout << "  PASSED: Custom rule added successfully" << std::endl;
}

void cleanup() {
    auto& tableMgr = storage::TableManager::getInstance();
    tableMgr.dropDatabase("test_rule_db");
}

int main() {
    std::cout << "=== RuleOptimizer Module Tests ===" << std::endl;

    setupTestTable();

    testOptimizerSingleton();
    testOptimizeNullPlan();
    testOptimizeSimplePlan();
    testAddCustomRule();

    cleanup();

    std::cout << "\n=== All RuleOptimizer Tests PASSED ===" << std::endl;
    return 0;
}
