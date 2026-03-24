#include "../src/storage/TableManager.h"
#include "../src/storage/IndexManager.h"
#include "../src/storage/FileIO.h"
#include "../src/common/Type.h"
#include "../src/common/Value.h"
#include "../src/common/Logger.h"
#include <iostream>
#include <limits>

using namespace minisql;
using namespace storage;

int main() {
    std::cout << "=== 演示：存储引擎文件生成 ===" << std::endl;

    std::string dbName = "demo_db";
    std::string tableName = "users";

    // 1. 创建数据库
    std::cout << "\n1. 创建数据库..." << std::endl;
    auto result = TableManager::getInstance().createDatabase(dbName);
    if (result.isError()) {
        std::cerr << "错误: " << result.getError().getMessage() << std::endl;
        return 1;
    }
    std::cout << "   数据库创建成功: " << FileIO::getDatabaseDir(dbName) << std::endl;

    // 2. 创建表
    std::cout << "\n2. 创建表..." << std::endl;
    TableDef tableDef;
    tableDef.name = tableName;
    tableDef.database = dbName;
    tableDef.engine = "MiniSQL";

    ColumnDef col1;
    col1.name = "id";
    col1.type = DataType::INT;
    col1.primaryKey = true;
    tableDef.columns.push_back(col1);

    ColumnDef col2;
    col2.name = "name";
    col2.type = DataType::VARCHAR;
    col2.length = 50;
    col2.notNull = true;
    tableDef.columns.push_back(col2);

    ColumnDef col3;
    col3.name = "age";
    col3.type = DataType::INT;
    tableDef.columns.push_back(col3);

    auto createResult = TableManager::getInstance().createTable(dbName, tableDef);
    if (createResult.isError()) {
        std::cerr << "错误: " << createResult.getError().getMessage() << std::endl;
        return 1;
    }
    std::cout << "   表创建成功" << std::endl;

    // 3. 显示生成的文件
    std::cout << "\n3. 生成的文件:" << std::endl;
    std::string metaPath = FileIO::join(FileIO::getDatabaseDir(dbName), tableName + ".meta");
    std::string dataPath = FileIO::join(FileIO::getDatabaseDir(dbName), tableName + ".data");
    std::string rowidPath = FileIO::join(FileIO::getDatabaseDir(dbName), tableName + ".rowid");
    std::string idxPath = FileIO::join(FileIO::getDatabaseDir(dbName), tableName + ".idx");

    std::cout << "   " << metaPath << std::endl;
    std::cout << "   " << dataPath << std::endl;
    std::cout << "   " << rowidPath << std::endl;

    // 4. 显示 .meta 文件内容
    std::cout << "\n4. .meta 文件内容:" << std::endl;
    std::cout << "----------------------------------------" << std::endl;
    std::cout << FileIO::readFromFile(metaPath) << std::endl;
    std::cout << "----------------------------------------" << std::endl;

    // 5. 插入数据
    std::cout << "\n5. 插入数据..." << std::endl;
    Row row1;
    row1.push_back(Value(1));
    row1.push_back(Value("Alice"));
    row1.push_back(Value(25));
    TableManager::getInstance().insertRow(dbName, tableName, row1);
    std::cout << "   插入: Alice, 25" << std::endl;

    Row row2;
    row2.push_back(Value(2));
    row2.push_back(Value("Bob"));
    row2.push_back(Value());  // NULL age
    TableManager::getInstance().insertRow(dbName, tableName, row2);
    std::cout << "   插入: Bob, NULL" << std::endl;

    // 6. 显示 .data 文件内容
    std::cout << "\n6. .data 文件内容:" << std::endl;
    std::cout << "----------------------------------------" << std::endl;
    std::cout << FileIO::readFromFile(dataPath) << std::endl;
    std::cout << "----------------------------------------" << std::endl;

    // 7. 创建索引
    std::cout << "\n7. 创建索引..." << std::endl;
    auto idxResult = IndexManager::getInstance().createIndex(
        dbName, tableName, "idx_name", "name", IndexUsageType::UNIQUE);
    if (idxResult.isError()) {
        std::cerr << "   索引创建失败: " << idxResult.getError().getMessage() << std::endl;
    } else {
        std::cout << "   索引创建成功: " << idxPath << std::endl;
    }

    // 8. 显示 .idx 文件内容
    std::cout << "\n8. .idx 文件内容:" << std::endl;
    std::cout << "----------------------------------------" << std::endl;
    std::cout << FileIO::readFromFile(idxPath) << std::endl;
    std::cout << "----------------------------------------" << std::endl;

    // 9. 验证读取
    std::cout << "\n9. 验证数据读取..." << std::endl;
    auto loadResult = TableManager::getInstance().loadTable(dbName, tableName);
    if (loadResult.isSuccess()) {
        TableData data = *loadResult.getValue();
        std::cout << "   读取到 " << data.size() << " 行数据" << std::endl;
        for (const auto& pair : data) {
            const Row& row = pair.second;
            std::cout << "   Row " << pair.first << ": ";
            std::cout << row[0].getInt() << ", ";
            std::cout << row[1].getString() << ", ";
            if (row[2].isNull()) {
                std::cout << "NULL";
            } else {
                std::cout << row[2].getInt();
            }
            std::cout << std::endl;
        }
    }

    // 10. 验证索引查找
    std::cout << "\n10. 验证索引查找..." << std::endl;
    auto getResult = IndexManager::getInstance().getIndex(dbName, "idx_name");
    if (getResult.isSuccess()) {
        auto tree = *getResult.getValue();
        std::cout << "   索引查找成功" << std::endl;
    }

    std::cout << "\n=== 演示完成 ===" << std::endl;
    std::cout << "\n数据库保留在: " << FileIO::getDatabaseDir(dbName) << std::endl;
    std::cout << "可以手动查看生成的文件内容" << std::endl;
    std::cout << "\n生成的文件:" << std::endl;
    std::cout << "  - " << metaPath << std::endl;
    std::cout << "  - " << dataPath << std::endl;
    std::cout << "  - " << rowidPath << std::endl;
    std::cout << "  - " << idxPath << std::endl;
    std::cout << "\n按回车键退出..." << std::endl;
    std::cin.get();

    return 0;
}
