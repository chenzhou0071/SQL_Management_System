#include "../src/storage/FileIO.h"
#include "../src/storage/TableManager.h"
#include "../src/storage/IndexManager.h"
#include "../src/storage/BPlusTree.h"
#include "../src/common/Logger.h"
#include "../src/common/Type.h"
#include "../src/common/Value.h"
#include "test_utils.h"
#include <iostream>
#include <cassert>

using namespace minisql;
using namespace storage;

// ============================================================
// FileIO 测试
// ============================================================
TEST(test_fileio_path_operations) {
    std::string dataDir = FileIO::getDataDir();
    ASSERT_FALSE(dataDir.empty());

    std::string dbDir = FileIO::getDatabaseDir("testdb");
    ASSERT_TRUE(dbDir.find("testdb") != std::string::npos);

    std::string joined = FileIO::join(dataDir, "test.db");
    ASSERT_TRUE(joined.find("test.db") != std::string::npos);
}

TEST(test_fileio_directory_operations) {
    std::string testPath = FileIO::join(FileIO::getDataDir(), "unit_test_db");

    bool created = FileIO::createDirectory(testPath);
    ASSERT_TRUE(created);

    bool exists = FileIO::exists(testPath);
    ASSERT_TRUE(exists);

    bool isDir = FileIO::isDirectory(testPath);
    ASSERT_TRUE(isDir);

    bool removed = FileIO::removeDirectory(testPath);
    ASSERT_TRUE(removed);

    bool notExists = FileIO::exists(testPath);
    ASSERT_FALSE(notExists);
}

TEST(test_fileio_file_operations) {
    std::string testPath = FileIO::join(FileIO::getDataDir(), "unit_test_file.txt");
    std::string content = "Hello, MiniSQL!";

    bool written = FileIO::writeToFile(testPath, content);
    ASSERT_TRUE(written);

    std::string readContent = FileIO::readFromFile(testPath);
    ASSERT_STREQ(readContent.c_str(), content.c_str());

    std::string appended = " Appended text";
    bool appendResult = FileIO::appendToFile(testPath, appended);
    ASSERT_TRUE(appendResult);

    std::string fullContent = FileIO::readFromFile(testPath);
    ASSERT_TRUE(fullContent.find(content) != std::string::npos);
    ASSERT_TRUE(fullContent.find(appended) != std::string::npos);

    bool fileExists = FileIO::existsFile(testPath);
    ASSERT_TRUE(fileExists);

    bool removed = FileIO::removeFile(testPath);
    ASSERT_TRUE(removed);

    bool fileNotExists = FileIO::existsFile(testPath);
    ASSERT_FALSE(fileNotExists);
}

// ============================================================
// TableManager 测试
// ============================================================
TEST(test_tablemanager_database_operations) {
    std::string dbName = "unittest_db";

    auto createResult = TableManager::getInstance().createDatabase(dbName);
    ASSERT_TRUE(createResult.isSuccess());

    bool exists = TableManager::getInstance().databaseExists(dbName);
    ASSERT_TRUE(exists);

    auto listResult = TableManager::getInstance().listDatabases();
    ASSERT_TRUE(listResult.isSuccess());
    bool found = false;
    for (const auto& db : *listResult.getValue()) {
        if (db == dbName) {
            found = true;
            break;
        }
    }
    ASSERT_TRUE(found);

    auto dupResult = TableManager::getInstance().createDatabase(dbName);
    ASSERT_TRUE(dupResult.isError());

    auto dropResult = TableManager::getInstance().dropDatabase(dbName);
    ASSERT_TRUE(dropResult.isSuccess());

    bool notExists = TableManager::getInstance().databaseExists(dbName);
    ASSERT_FALSE(notExists);
}

TEST(test_tablemanager_table_operations) {
    std::string dbName = "unittest_table_db";
    std::string tableName = "users";

    TableManager::getInstance().createDatabase(dbName);

    TableDef tableDef;
    tableDef.name = tableName;
    tableDef.database = dbName;
    tableDef.comment = "Test table";

    ColumnDef col1;
    col1.name = "id";
    col1.type = DataType::INT;
    col1.notNull = true;
    col1.primaryKey = true;
    col1.autoIncrement = true;
    tableDef.columns.push_back(col1);

    ColumnDef col2;
    col2.name = "name";
    col2.type = DataType::VARCHAR;
    col2.length = 100;
    col2.notNull = true;
    tableDef.columns.push_back(col2);

    ColumnDef col3;
    col3.name = "age";
    col3.type = DataType::INT;
    col3.notNull = false;
    col3.hasDefault = true;
    col3.defaultValue = "18";
    tableDef.columns.push_back(col3);

    auto createResult = TableManager::getInstance().createTable(dbName, tableDef);
    ASSERT_TRUE(createResult.isSuccess());

    auto existsResult = TableManager::getInstance().tableExists(dbName, tableName);
    ASSERT_TRUE(existsResult.isSuccess());
    ASSERT_TRUE(*existsResult.getValue());

    auto listResult = TableManager::getInstance().listTables(dbName);
    ASSERT_TRUE(listResult.isSuccess());
    bool tableFound = false;
    for (const auto& t : *listResult.getValue()) {
        if (t == tableName) {
            tableFound = true;
            break;
        }
    }
    ASSERT_TRUE(tableFound);

    auto defResult = TableManager::getInstance().getTableDef(dbName, tableName);
    ASSERT_TRUE(defResult.isSuccess());
    ASSERT_STREQ(defResult.getValue()->name.c_str(), tableName.c_str());
    ASSERT_EQ(defResult.getValue()->columns.size(), 3);
    ASSERT_STREQ(defResult.getValue()->columns[0].name.c_str(), "id");
    ASSERT_STREQ(defResult.getValue()->columns[1].name.c_str(), "name");
    ASSERT_STREQ(defResult.getValue()->columns[2].name.c_str(), "age");

    auto dropResult = TableManager::getInstance().dropTable(dbName, tableName);
    ASSERT_TRUE(dropResult.isSuccess());

    TableManager::getInstance().dropDatabase(dbName);
}

TEST(test_tablemanager_row_operations) {
    std::string dbName = "unittest_row_db";
    std::string tableName = "test_table";

    TableManager::getInstance().createDatabase(dbName);

    TableDef tableDef;
    tableDef.name = tableName;
    tableDef.database = dbName;

    ColumnDef col1;
    col1.name = "id";
    col1.type = DataType::INT;
    col1.notNull = true;
    col1.primaryKey = true;
    tableDef.columns.push_back(col1);

    ColumnDef col2;
    col2.name = "name";
    col2.type = DataType::VARCHAR;
    col2.length = 50;
    col2.notNull = true;
    tableDef.columns.push_back(col2);

    TableManager::getInstance().createTable(dbName, tableDef);

    Row row1;
    row1.push_back(Value(1));
    row1.push_back(Value("Alice"));

    auto insertResult = TableManager::getInstance().insertRow(dbName, tableName, row1);
    ASSERT_TRUE(insertResult.isSuccess());

    Row row2;
    row2.push_back(Value(2));
    row2.push_back(Value("Bob"));

    auto insertResult2 = TableManager::getInstance().insertRow(dbName, tableName, row2);
    ASSERT_TRUE(insertResult2.isSuccess());

    auto loadResult = TableManager::getInstance().loadTable(dbName, tableName);
    ASSERT_TRUE(loadResult.isSuccess());
    ASSERT_EQ(loadResult.getValue()->size(), 2);

    Row updatedRow;
    updatedRow.push_back(Value(1));
    updatedRow.push_back(Value("Alice Updated"));

    auto updateResult = TableManager::getInstance().updateRow(dbName, tableName, 1, updatedRow);
    ASSERT_TRUE(updateResult.isSuccess());

    auto deleteResult = TableManager::getInstance().deleteRow(dbName, tableName, 2);
    ASSERT_TRUE(deleteResult.isSuccess());

    auto verifyResult = TableManager::getInstance().loadTable(dbName, tableName);
    ASSERT_TRUE(verifyResult.isSuccess());
    ASSERT_EQ(verifyResult.getValue()->size(), 1);

    TableManager::getInstance().dropDatabase(dbName);
}

TEST(test_tablemanager_rowid_generation) {
    std::string dbName = "unittest_rowid_db";
    std::string tableName = "id_test";

    TableManager::getInstance().createDatabase(dbName);

    TableDef tableDef;
    tableDef.name = tableName;
    tableDef.database = dbName;

    ColumnDef col1;
    col1.name = "value";
    col1.type = DataType::INT;
    tableDef.columns.push_back(col1);

    TableManager::getInstance().createTable(dbName, tableDef);

    auto id1Result = TableManager::getInstance().getNextRowId(dbName, tableName);
    ASSERT_TRUE(id1Result.isSuccess());
    ASSERT_EQ(*id1Result.getValue(), 1);

    auto id2Result = TableManager::getInstance().getNextRowId(dbName, tableName);
    ASSERT_TRUE(id2Result.isSuccess());
    ASSERT_EQ(*id2Result.getValue(), 2);

    auto id3Result = TableManager::getInstance().getNextRowId(dbName, tableName);
    ASSERT_TRUE(id3Result.isSuccess());
    ASSERT_EQ(*id3Result.getValue(), 3);

    TableManager::getInstance().dropDatabase(dbName);
}

// ============================================================
// 完整表结构测试（验证 .meta 和 .data 文件格式）
// ============================================================
TEST(test_table_full_structure_with_indexes_and_fkeys) {
    std::string dbName = "verify_meta";
    std::string tableName = "users";

    // 创建数据库
    TableManager::getInstance().createDatabase(dbName);

    // 按设计文档创建表
    TableDef tableDef;
    tableDef.name = tableName;
    tableDef.database = dbName;
    tableDef.engine = "MiniSQL";
    tableDef.comment = "User information table";

    // id 列
    ColumnDef col1;
    col1.name = "id";
    col1.type = DataType::INT;
    col1.primaryKey = true;
    tableDef.columns.push_back(col1);

    // name 列
    ColumnDef col2;
    col2.name = "name";
    col2.type = DataType::VARCHAR;
    col2.length = 50;
    col2.notNull = true;
    tableDef.columns.push_back(col2);

    // age 列
    ColumnDef col3;
    col3.name = "age";
    col3.type = DataType::INT;
    col3.hasDefault = true;
    col3.defaultValue = "0";
    tableDef.columns.push_back(col3);

    // email 列
    ColumnDef col4;
    col4.name = "email";
    col4.type = DataType::VARCHAR;
    col4.length = 100;
    col4.unique = true;
    tableDef.columns.push_back(col4);

    // dept_id 列（外键）
    ColumnDef col5;
    col5.name = "dept_id";
    col5.type = DataType::INT;
    tableDef.columns.push_back(col5);

    // 添加主键索引
    IndexDef pkIdx;
    pkIdx.name = "PRIMARY";
    pkIdx.tableName = tableName;
    pkIdx.columns = {"id"};
    pkIdx.type = IndexType::BTREE;
    pkIdx.unique = true;
    tableDef.indexes.push_back(pkIdx);

    // 添加 email 唯一索引
    IndexDef emailIdx;
    emailIdx.name = "idx_email";
    emailIdx.tableName = tableName;
    emailIdx.columns = {"email"};
    emailIdx.type = IndexType::BTREE;
    emailIdx.unique = true;
    tableDef.indexes.push_back(emailIdx);

    // 添加复合索引
    IndexDef compositeIdx;
    compositeIdx.name = "idx_name_age";
    compositeIdx.tableName = tableName;
    compositeIdx.columns = {"name", "age"};
    compositeIdx.type = IndexType::BTREE;
    compositeIdx.unique = false;
    tableDef.indexes.push_back(compositeIdx);

    // 添加外键
    ForeignKeyDef fk;
    fk.name = "fk_dept";
    fk.column = "dept_id";
    fk.refTable = "departments";
    fk.refColumn = "id";
    fk.onDelete = "CASCADE";
    fk.onUpdate = "CASCADE";
    tableDef.foreignKeys.push_back(fk);

    // 创建表
    auto createResult = TableManager::getInstance().createTable(dbName, tableDef);
    ASSERT_TRUE(createResult.isSuccess());

    // 验证 .meta 文件存在
    std::string metaPath = FileIO::join(FileIO::getDatabaseDir(dbName), tableName + ".meta");
    ASSERT_TRUE(FileIO::existsFile(metaPath));

    // 验证 .data 文件存在
    std::string dataPath = FileIO::join(FileIO::getDatabaseDir(dbName), tableName + ".data");
    ASSERT_TRUE(FileIO::existsFile(dataPath));

    // 验证 .rowid 文件存在
    std::string rowidPath = FileIO::join(FileIO::getDatabaseDir(dbName), tableName + ".rowid");
    ASSERT_TRUE(FileIO::existsFile(rowidPath));

    // 重新加载表定义验证完整性
    auto defResult = TableManager::getInstance().getTableDef(dbName, tableName);
    ASSERT_TRUE(defResult.isSuccess());

    TableDef loadedDef = *defResult.getValue();
    ASSERT_STREQ(loadedDef.name.c_str(), tableName.c_str());
    ASSERT_STREQ(loadedDef.engine.c_str(), "MiniSQL");

    // 验证列
    ASSERT_EQ(loadedDef.columns.size(), 5);
    ASSERT_STREQ(loadedDef.columns[0].name.c_str(), "id");
    ASSERT_TRUE(loadedDef.columns[0].primaryKey);
    ASSERT_STREQ(loadedDef.columns[1].name.c_str(), "name");
    ASSERT_EQ(loadedDef.columns[1].length, 50);
    ASSERT_TRUE(loadedDef.columns[1].notNull);
    ASSERT_STREQ(loadedDef.columns[2].name.c_str(), "age");
    ASSERT_TRUE(loadedDef.columns[2].hasDefault);
    ASSERT_STREQ(loadedDef.columns[2].defaultValue.c_str(), "0");
    ASSERT_STREQ(loadedDef.columns[3].name.c_str(), "email");
    ASSERT_EQ(loadedDef.columns[3].length, 100);
    ASSERT_TRUE(loadedDef.columns[3].unique);
    ASSERT_STREQ(loadedDef.columns[4].name.c_str(), "dept_id");

    // 验证索引
    ASSERT_EQ(loadedDef.indexes.size(), 3);
    ASSERT_STREQ(loadedDef.indexes[0].name.c_str(), "PRIMARY");
    ASSERT_TRUE(loadedDef.indexes[0].unique);
    ASSERT_EQ(loadedDef.indexes[0].columns.size(), 1);
    ASSERT_STREQ(loadedDef.indexes[0].columns[0].c_str(), "id");

    ASSERT_STREQ(loadedDef.indexes[1].name.c_str(), "idx_email");
    ASSERT_TRUE(loadedDef.indexes[1].unique);
    ASSERT_EQ(loadedDef.indexes[1].columns.size(), 1);
    ASSERT_STREQ(loadedDef.indexes[1].columns[0].c_str(), "email");

    ASSERT_STREQ(loadedDef.indexes[2].name.c_str(), "idx_name_age");
    ASSERT_FALSE(loadedDef.indexes[2].unique);
    ASSERT_EQ(loadedDef.indexes[2].columns.size(), 2);
    ASSERT_STREQ(loadedDef.indexes[2].columns[0].c_str(), "name");
    ASSERT_STREQ(loadedDef.indexes[2].columns[1].c_str(), "age");

    // 验证外键
    ASSERT_EQ(loadedDef.foreignKeys.size(), 1);
    ASSERT_STREQ(loadedDef.foreignKeys[0].name.c_str(), "fk_dept");
    ASSERT_STREQ(loadedDef.foreignKeys[0].column.c_str(), "dept_id");
    ASSERT_STREQ(loadedDef.foreignKeys[0].refTable.c_str(), "departments");
    ASSERT_STREQ(loadedDef.foreignKeys[0].refColumn.c_str(), "id");
    ASSERT_STREQ(loadedDef.foreignKeys[0].onDelete.c_str(), "CASCADE");
    ASSERT_STREQ(loadedDef.foreignKeys[0].onUpdate.c_str(), "CASCADE");

    // 测试数据插入和 .data 文件格式
    Row row1;
    row1.push_back(Value(1));
    row1.push_back(Value("Alice"));
    row1.push_back(Value(20));
    row1.push_back(Value("alice@example.com"));
    row1.push_back(Value(1));

    auto insert1 = TableManager::getInstance().insertRow(dbName, tableName, row1);
    ASSERT_TRUE(insert1.isSuccess());

    // 插入带 NULL 值的行
    Row row2;
    row2.push_back(Value(2));
    row2.push_back(Value("Bob"));
    row2.push_back(Value());  // NULL age
    row2.push_back(Value("bob@example.com"));
    row2.push_back(Value(2));

    auto insert2 = TableManager::getInstance().insertRow(dbName, tableName, row2);
    ASSERT_TRUE(insert2.isSuccess());

    // 验证数据文件内容
    std::string dataContent = FileIO::readFromFile(dataPath);
    ASSERT_FALSE(dataContent.empty());

    // 验证数据能正确加载
    auto loadResult = TableManager::getInstance().loadTable(dbName, tableName);
    ASSERT_TRUE(loadResult.isSuccess());
    ASSERT_EQ(loadResult.getValue()->size(), 2);

    TableData data = *loadResult.getValue();
    ASSERT_EQ(data[1][0].getInt(), 1);
    ASSERT_STREQ(data[1][1].getString().c_str(), "Alice");
    ASSERT_EQ(data[1][2].getInt(), 20);

    // 验证 NULL 值
    ASSERT_EQ(data[2][0].getInt(), 2);
    ASSERT_STREQ(data[2][1].getString().c_str(), "Bob");
    ASSERT_TRUE(data[2][2].isNull());  // age is NULL

    // 清理
    TableManager::getInstance().dropDatabase(dbName);
}

// ============================================================
// B+Tree 测试
// ============================================================
TEST(test_bplus_tree_basic_operations) {
    BPlusTree tree(4);

    ASSERT_TRUE(tree.insert(10, 1));
    ASSERT_TRUE(tree.insert(20, 2));
    ASSERT_TRUE(tree.insert(30, 3));

    ASSERT_EQ(tree.find(10), 1);
    ASSERT_EQ(tree.find(20), 2);
    ASSERT_EQ(tree.find(30), 3);
    ASSERT_EQ(tree.find(999), -1);

    ASSERT_TRUE(tree.remove(20));
    ASSERT_EQ(tree.find(20), -1);

    ASSERT_FALSE(tree.insert(10, 1));  // 重复插入
}

TEST(test_bplus_tree_split) {
    BPlusTree tree(4);

    ASSERT_TRUE(tree.insert(10, 1));
    ASSERT_TRUE(tree.insert(20, 2));
    ASSERT_TRUE(tree.insert(30, 3));
    ASSERT_TRUE(tree.insert(15, 4));
    ASSERT_TRUE(tree.insert(5, 5));

    ASSERT_EQ(tree.find(10), 1);
    ASSERT_EQ(tree.find(20), 2);
    ASSERT_EQ(tree.find(30), 3);
    ASSERT_EQ(tree.find(15), 4);
    ASSERT_EQ(tree.find(5), 5);
    ASSERT_EQ(tree.find(999), -1);

    std::vector<int> range = tree.rangeSearch(10, 25);
    ASSERT_GE(range.size(), 2);
}

TEST(test_bplus_tree_empty) {
    BPlusTree tree(4);

    ASSERT_EQ(tree.find(10), -1);

    std::vector<int> range = tree.rangeSearch(1, 100);
    ASSERT_EQ(range.size(), 0);
}

TEST(test_bplus_tree_large_order) {
    BPlusTree tree(200);

    for (int i = 0; i < 1000; i++) {
        ASSERT_TRUE(tree.insert(i * 10, i));
    }

    ASSERT_EQ(tree.find(0), 0);
    ASSERT_EQ(tree.find(5000), 500);
    ASSERT_EQ(tree.find(9990), 999);
    ASSERT_EQ(tree.find(-1), -1);

    std::vector<int> range = tree.rangeSearch(1000, 3000);
    ASSERT_GE(range.size(), 2);
}

// ============================================================
// IndexManager 测试
// ============================================================
TEST(test_indexmanager_basic) {
    std::string dbName = "unittest_index_db";
    std::string tableName = "idx_test";

    TableManager::getInstance().createDatabase(dbName);

    TableDef tableDef;
    tableDef.name = tableName;
    tableDef.database = dbName;

    ColumnDef col1;
    col1.name = "id";
    col1.type = DataType::INT;
    col1.primaryKey = true;
    tableDef.columns.push_back(col1);

    ColumnDef col2;
    col2.name = "value";
    col2.type = DataType::INT;
    tableDef.columns.push_back(col2);

    TableManager::getInstance().createTable(dbName, tableDef);

    Row row1;
    row1.push_back(Value(1));
    row1.push_back(Value(100));
    TableManager::getInstance().insertRow(dbName, tableName, row1);

    Row row2;
    row2.push_back(Value(2));
    row2.push_back(Value(200));
    TableManager::getInstance().insertRow(dbName, tableName, row2);

    auto createResult = IndexManager::getInstance().createIndex(
        dbName, tableName, "idx_value", "value", IndexUsageType::NORMAL);
    ASSERT_TRUE(createResult.isSuccess());

    auto getResult = IndexManager::getInstance().getIndex(dbName, "idx_value");
    ASSERT_TRUE(getResult.isSuccess());
    ASSERT_NOT_NULL(getResult.getValue()->get());

    auto dropResult = IndexManager::getInstance().dropIndex(dbName, "idx_value");
    ASSERT_TRUE(dropResult.isSuccess());

    TableManager::getInstance().dropDatabase(dbName);
}

// ============================================================
// 主函数
// ============================================================
int main() {
    std::cout << "======================================\n";
    std::cout << "  Storage Module Tests (Phase 2)\n";
    std::cout << "======================================\n\n";

    // FileIO 测试
    std::cout << "--- FileIO Tests ---\n";
    test_fileio_path_operations();
    test_fileio_directory_operations();
    test_fileio_file_operations();

    // TableManager 测试
    std::cout << "\n--- TableManager Tests ---\n";
    test_tablemanager_database_operations();
    test_tablemanager_table_operations();
    test_tablemanager_row_operations();
    test_tablemanager_rowid_generation();

    // 完整表结构测试（验证 .meta 和 .data 格式）
    std::cout << "\n--- Table Structure & File Format Tests ---\n";
    test_table_full_structure_with_indexes_and_fkeys();

    // B+Tree 测试
    std::cout << "\n--- B+Tree Tests ---\n";
    test_bplus_tree_basic_operations();
    test_bplus_tree_split();
    test_bplus_tree_empty();
    test_bplus_tree_large_order();

    // IndexManager 测试
    std::cout << "\n--- IndexManager Tests ---\n";
    test_indexmanager_basic();

    RUN_TESTS;

    std::cout << "\n======================================\n";
    std::cout << "  All tests completed!\n";
    std::cout << "======================================\n";

    std::cout << "\n按回车键退出..." << std::endl;
    std::cin.get();
    return 0;
}
