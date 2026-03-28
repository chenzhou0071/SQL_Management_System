#include "../src/optimizer/ExplainHandler.h"
#include "../src/optimizer/PlanGenerator.h"
#include "../src/storage/TableManager.h"
#include <cassert>
#include <iostream>
#include <memory>

using namespace minisql;

void setupTestTable() {
    auto& tableMgr = storage::TableManager::getInstance();

    if (tableMgr.databaseExists("test_explain_db")) {
        tableMgr.dropDatabase("test_explain_db");
    }
    tableMgr.createDatabase("test_explain_db");

    TableDef tableDef;
    tableDef.name = "users";
    tableDef.database = "test_explain_db";

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

    tableDef.columns.push_back(idCol);
    tableDef.columns.push_back(ageCol);
    tableDef.columns.push_back(nameCol);

    IndexDef idxAge;
    idxAge.name = "idx_age";
    idxAge.tableName = "users";
    idxAge.columns.push_back("age");
    tableDef.indexes.push_back(idxAge);

    tableMgr.createTable("test_explain_db", tableDef);

    for (int i = 1; i <= 100; ++i) {
        std::string name = "User" + std::to_string(i);
        tableMgr.insertRow("test_explain_db", "users",
            {Value(i), Value(18 + (i % 50)), Value(name)});
    }
}

void testExplainRowCreation() {
    std::cout << "Running testExplainRowCreation..." << std::endl;

    optimizer::ExplainRow row;
    row.id = 1;
    row.selectType = "SIMPLE";
    row.table = "users";
    row.type = "ALL";
    row.key = "";
    row.rows = 100;
    row.extra.push_back("Using filesort");

    std::string str = row.toString();
    assert(!str.empty());
    assert(str.find("id=1") != std::string::npos);
    assert(str.find("users") != std::string::npos);

    std::cout << "  ExplainRow: " << str << std::endl;
    std::cout << "  PASSED" << std::endl;
}

void testGetAccessType() {
    std::cout << "Running testGetAccessType..." << std::endl;

    using namespace optimizer;

    // 全表扫描
    std::string type1 = ExplainHandler::getAccessType(ScanType::FULL_SCAN, false, 0.5);
    assert(type1 == "ALL");

    // 覆盖索引 + 等值
    std::string type2 = ExplainHandler::getAccessType(ScanType::COVERING_SCAN, true, 0.1);
    assert(type2 == "ref");

    // 覆盖索引 + 范围
    std::string type3 = ExplainHandler::getAccessType(ScanType::COVERING_SCAN, false, 0.5);
    assert(type3 == "range");

    // 索引扫描 + 高选择性
    std::string type4 = ExplainHandler::getAccessType(ScanType::INDEX_SCAN, true, 0.001);
    assert(type4 == "const");

    // 索引扫描 + 等值
    std::string type5 = ExplainHandler::getAccessType(ScanType::INDEX_SCAN, true, 0.1);
    assert(type5 == "eq_ref");

    std::cout << "  PASSED" << std::endl;
}

void testExplainWithPlan() {
    std::cout << "Running testExplainWithPlan..." << std::endl;

    auto plan = std::make_shared<optimizer::PlanNode>();
    plan->nodeId = "id_1";
    plan->operatorType = "TableScan";
    plan->tableName = "users";
    plan->scanType = optimizer::ScanType::FULL_SCAN;
    plan->estimatedRows = 100;

    auto rows = optimizer::ExplainHandler::explainPlan(plan);
    assert(rows.size() == 1);
    assert(rows[0].table == "users");
    assert(rows[0].type == "ALL");
    assert(rows[0].rows == 100);

    std::cout << "  PASSED" << std::endl;
}

void testExplainSelectStmt() {
    std::cout << "Running testExplainSelectStmt..." << std::endl;

    auto stmt = new parser::SelectStmt();

    auto colRef = std::make_shared<parser::ColumnRef>("", "*");
    stmt->selectItems.push_back(colRef);

    auto left = std::make_shared<parser::ColumnRef>("", "age");
    auto right = std::make_shared<parser::LiteralExpr>(Value(25));
    stmt->whereClause = std::make_shared<parser::BinaryExpr>(left, ">", right);

    auto tableRef = std::make_shared<parser::TableRef>();
    tableRef->name = "users";
    stmt->fromTable = tableRef;

    auto result = optimizer::ExplainHandler::explain("test_explain_db", stmt);
    assert(result.isSuccess());

    auto rows = result.getValue();
    std::cout << "  Generated " << rows->size() << " explain row(s)" << std::endl;

    for (const auto& row : *rows) {
        std::cout << "    " << row.toString() << std::endl;
    }

    std::cout << "  PASSED" << std::endl;
    delete stmt;
}

void testFormatTable() {
    std::cout << "Running testFormatTable..." << std::endl;

    std::vector<optimizer::ExplainRow> rows;
    optimizer::ExplainRow row1;
    row1.id = 1;
    row1.selectType = "SIMPLE";
    row1.table = "users";
    row1.type = "ALL";
    row1.rows = 100;
    rows.push_back(row1);

    optimizer::ExplainRow row2;
    row2.id = 2;
    row2.selectType = "SIMPLE";
    row2.table = "orders";
    row2.type = "ref";
    row2.key = "idx_user_id";
    row2.rows = 10;
    rows.push_back(row2);

    std::string table = optimizer::ExplainHandler::formatTable(rows);
    assert(!table.empty());
    assert(table.find("id") != std::string::npos);
    assert(table.find("users") != std::string::npos);
    assert(table.find("orders") != std::string::npos);

    std::cout << "  Formatted table:\n" << table << std::endl;
    std::cout << "  PASSED" << std::endl;
}

void testFormatJSON() {
    std::cout << "Running testFormatJSON..." << std::endl;

    std::vector<optimizer::ExplainRow> rows;
    optimizer::ExplainRow row;
    row.id = 1;
    row.selectType = "SIMPLE";
    row.table = "users";
    row.type = "ALL";
    row.rows = 100;
    rows.push_back(row);

    std::string json = optimizer::ExplainHandler::formatJSON(rows);
    assert(!json.empty());
    assert(json.find("[") != std::string::npos);
    assert(json.find("\"id\": 1") != std::string::npos);
    assert(json.find("\"table\": \"users\"") != std::string::npos);

    std::cout << "  Formatted JSON:\n" << json << std::endl;
    std::cout << "  PASSED" << std::endl;
}

void testBuildExtra() {
    std::cout << "Running testBuildExtra..." << std::endl;

    // 测试覆盖索引
    auto coveringPlan = std::make_shared<optimizer::PlanNode>();
    coveringPlan->scanType = optimizer::ScanType::COVERING_SCAN;

    auto rows1 = optimizer::ExplainHandler::explainPlan(coveringPlan);
    assert(!rows1[0].extra.empty());

    // 测试排序节点
    auto sortPlan = std::make_shared<optimizer::PlanNode>();
    sortPlan->operatorType = "Sort";
    sortPlan->children.push_back(coveringPlan);

    auto rows2 = optimizer::ExplainHandler::explainPlan(sortPlan);
    assert(rows2.size() == 2);

    std::cout << "  PASSED" << std::endl;
}

void cleanup() {
    auto& tableMgr = storage::TableManager::getInstance();
    tableMgr.dropDatabase("test_explain_db");
}

int main() {
    std::cout << "=== ExplainHandler Module Tests ===" << std::endl;

    setupTestTable();

    testExplainRowCreation();
    testGetAccessType();
    testExplainWithPlan();
    testExplainSelectStmt();
    testFormatTable();
    testFormatJSON();
    testBuildExtra();

    cleanup();

    std::cout << "\n=== All ExplainHandler Tests PASSED ===" << std::endl;
    return 0;
}
