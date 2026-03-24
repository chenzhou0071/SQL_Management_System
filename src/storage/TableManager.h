#pragma once

#include "../common/Type.h"
#include "../common/Value.h"
#include "../common/Error.h"
#include <string>
#include <vector>
#include <map>
#include <memory>

namespace minisql {
namespace storage {

// 行数据（列值的向量）
using Row = std::vector<Value>;

// 表数据（行 ID -> 行）
using TableData = std::map<int, Row>;

class TableManager {
public:
    static TableManager& getInstance();

    // 数据库操作
    Result<void> createDatabase(const std::string& dbName);
    Result<void> dropDatabase(const std::string& dbName);
    Result<std::vector<std::string>> listDatabases();
    bool databaseExists(const std::string& dbName);

    // 表操作
    Result<void> createTable(const std::string& dbName, const TableDef& tableDef);
    Result<void> dropTable(const std::string& dbName, const std::string& tableName);
    Result<bool> tableExists(const std::string& dbName, const std::string& tableName);
    Result<std::vector<std::string>> listTables(const std::string& dbName);
    Result<TableDef> getTableDef(const std::string& dbName, const std::string& tableName);

    // 数据操作
    Result<void> insertRow(const std::string& dbName, const std::string& tableName, const Row& row);
    Result<TableData> loadTable(const std::string& dbName, const std::string& tableName);
    Result<void> deleteRow(const std::string& dbName, const std::string& tableName, int rowId);
    Result<void> updateRow(const std::string& dbName, const std::string& tableName, int rowId, const Row& row);

    // 获取下一行 ID（自增）
    Result<int> getNextRowId(const std::string& dbName, const std::string& tableName);

private:
    TableManager() = default;
    ~TableManager() = default;

    TableManager(const TableManager&) = delete;
    TableManager& operator=(const TableManager&) = delete;

    // 文件路径
    std::string getMetaPath(const std::string& dbName, const std::string& tableName);
    std::string getDataPath(const std::string& dbName, const std::string& tableName);

    // JSON 序列化
    std::string tableDefToJson(const TableDef& tableDef);
    Result<TableDef> jsonToTableDef(const std::string& json);

    // CSV 序列化
    std::string rowToCsv(const Row& row);
    Result<Row> csvToRow(const std::string& csv, const TableDef& tableDef);

    // 行 ID 管理
    std::string getRowIdPath(const std::string& dbName, const std::string& tableName);
    Result<int> loadNextRowId(const std::string& dbName, const std::string& tableName);
    Result<void> saveNextRowId(const std::string& dbName, const std::string& tableName, int rowId);
};

}  // namespace storage
}  // namespace minisql
