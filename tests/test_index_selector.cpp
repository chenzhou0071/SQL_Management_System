#include "../src/optimizer/IndexSelector.h"
#include "../src/optimizer/Statistics.h"
#include "../src/storage/TableManager.h"
#include "../src/storage/IndexManager.h"
#include "../src/parser/AST.h"
#include "../src/common/Type.h"
#include <cassert>
#include <iostream>
#include <memory>

using namespace minisql;

void setupTestTable() {
    auto& tableMgr = storage::TableManager::getInstance();

    // 创建测试数据库
    if (tableMgr.databaseExists("test_index_db")) {
        tableMgr.dropDatabase("test_index_db");
    }
    tableMgr.createDatabase("test_index_db");

    // 创建表
    TableDef tableDef;
    tableDef.name = "products";
    tableDef.database = "test_index_db";

    ColumnDef idCol;
    idCol.name = "id";
    idCol.type = DataType::INT;
    idCol.primaryKey = true;

    ColumnDef categoryCol;
    categoryCol.name = "category";
    categoryCol.type = DataType::INT;

    ColumnDef priceCol;
    priceCol.name = "price";
    priceCol.type = DataType::DOUBLE;

    ColumnDef nameCol;
    nameCol.name = "name";
    nameCol.type = DataType::VARCHAR;
    nameCol.length = 100;

    tableDef.columns.push_back(idCol);
    tableDef.columns.push_back(categoryCol);
    tableDef.columns.push_back(priceCol);
    tableDef.columns.push_back(nameCol);

    // 添加索引定义
    IndexDef idxCategory;
    idxCategory.name = "idx_category";
    idxCategory.tableName = "products";
    idxCategory.columns.push_back("category");
    tableDef.indexes.push_back(idxCategory);

    IndexDef idxPrice;
    idxPrice.name = "idx_price";
    idxPrice.tableName = "products";
    idxPrice.columns.push_back("price");
    tableDef.indexes.push_back(idxPrice);

    // 复合索引
    IndexDef idxCatPrice;
    idxCatPrice.name = "idx_cat_price";
    idxCatPrice.tableName = "products";
    idxCatPrice.columns.push_back("category");
    idxCatPrice.columns.push_back("price");
    tableDef.indexes.push_back(idxCatPrice);

    tableMgr.createTable("test_index_db", tableDef);

    // 插入测试数据
    for (int i = 1; i <= 50; ++i) {
        int category = (i % 5) + 1;  // 5 个类别
        double price = 10.0 + (i * 5.0);  // 价格递增
        std::string name = "Product" + std::to_string(i);
        tableMgr.insertRow("test_index_db", "products",
            {Value(i), Value(category), Value(price), Value(name)});
    }
}

void testGetAvailableIndexes() {
    std::cout << "Running testGetAvailableIndexes..." << std::endl;

    auto indexes = optimizer::IndexSelector::getAvailableIndexes("test_index_db", "products");
    assert(indexes.isSuccess() && "getAvailableIndexes should succeed");
    assert(indexes.getValue()->size() == 3 && "Should have 3 indexes");

    std::cout << "  PASSED: Found " << indexes.getValue()->size() << " indexes" << std::endl;
}

void testExtractColumns() {
    std::cout << "Running testExtractColumns..." << std::endl;

    // 构建测试表达式：category = 1
    auto left = std::make_shared<parser::ColumnRef>("", "category");
    auto right = std::make_shared<parser::LiteralExpr>(Value(1));
    auto expr = std::make_shared<parser::BinaryExpr>(left, "=", right);

    // 提取列
    auto columns = optimizer::IndexSelector::extractColumns(expr);
    assert(columns.size() == 1 && "Should extract 1 column");
    assert(columns[0] == "category" && "Should extract 'category' column");

    // 构建更复杂的表达式：category = 1 AND price > 10
    auto left2 = std::make_shared<parser::ColumnRef>("", "category");
    auto right2 = std::make_shared<parser::LiteralExpr>(Value(1));
    auto expr2 = std::make_shared<parser::BinaryExpr>(left2, "=", right2);

    auto left3 = std::make_shared<parser::ColumnRef>("", "price");
    auto right3 = std::make_shared<parser::LiteralExpr>(Value(10));
    auto expr3 = std::make_shared<parser::BinaryExpr>(left3, ">", right3);

    auto andExpr = std::make_shared<parser::BinaryExpr>(expr2, "AND", expr3);

    auto columns2 = optimizer::IndexSelector::extractColumns(andExpr);
    assert(columns2.size() == 2 && "Should extract 2 columns from AND expression");

    std::cout << "  PASSED: Extracted " << columns2.size() << " columns" << std::endl;
}

void testIsEqualityCondition() {
    std::cout << "Running testIsEqualityCondition..." << std::endl;

    // 测试等值条件
    auto left = std::make_shared<parser::ColumnRef>("", "category");
    auto right = std::make_shared<parser::LiteralExpr>(Value(1));
    auto eqExpr = std::make_shared<parser::BinaryExpr>(left, "=", right);
    assert(optimizer::IndexSelector::isEqualityCondition(eqExpr) && "= should be equality");

    // 测试范围条件
    auto left2 = std::make_shared<parser::ColumnRef>("", "price");
    auto right2 = std::make_shared<parser::LiteralExpr>(Value(10));
    auto gtExpr = std::make_shared<parser::BinaryExpr>(left2, ">", right2);
    assert(!optimizer::IndexSelector::isEqualityCondition(gtExpr) && "> should not be equality");

    std::cout << "  PASSED: Equality condition detection works" << std::endl;
}

void testIsIndexUsable() {
    std::cout << "Running testIsIndexUsable..." << std::endl;

    std::vector<std::string> indexCols = {"category", "price"};
    std::vector<std::string> queryCols1 = {"category"};
    std::vector<std::string> queryCols2 = {"category", "price"};
    std::vector<std::string> queryCols3 = {"price", "category"};  // 顺序不同但列相同
    std::vector<std::string> queryCols4 = {"name"};  // 不在索引中的列

    assert(optimizer::IndexSelector::isIndexUsable(indexCols, queryCols1) &&
           "Should be usable for subset");
    assert(optimizer::IndexSelector::isIndexUsable(indexCols, queryCols2) &&
           "Should be usable for exact match");
    // isIndexUsable 只检查列是否存在，不检查顺序
    assert(optimizer::IndexSelector::isIndexUsable(indexCols, queryCols3) &&
           "Should be usable regardless of order");
    assert(!optimizer::IndexSelector::isIndexUsable(indexCols, queryCols4) &&
           "Should not be usable for columns not in index");

    std::cout << "  PASSED: Index usability check works" << std::endl;
}

void cleanup() {
    auto& tableMgr = storage::TableManager::getInstance();
    tableMgr.dropDatabase("test_index_db");
}

int main() {
    std::cout << "=== IndexSelector Module Tests ===" << std::endl;

    setupTestTable();

    testGetAvailableIndexes();
    testExtractColumns();
    testIsEqualityCondition();
    testIsIndexUsable();

    cleanup();

    std::cout << "\n=== All IndexSelector Tests PASSED ===" << std::endl;
    return 0;
}
