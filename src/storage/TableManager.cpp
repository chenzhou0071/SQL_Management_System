#include "TableManager.h"
#include "FileIO.h"
#include "../common/Logger.h"
#include "../common/Value.h"
#include <fstream>
#include <sstream>
#include "external/json.hpp"

using namespace minisql;
using namespace storage;
using json = nlohmann::json;

TableManager& TableManager::getInstance() {
    static TableManager instance;
    return instance;
}

Result<void> TableManager::createDatabase(const std::string& dbName) {
    if (dbName.empty()) {
        return Result<void>(MiniSQLException(ErrorCode::STORAGE_FILE_IO_ERROR, "Database name cannot be empty"));
    }

    std::string dbDir = FileIO::getDatabaseDir(dbName);
    if (FileIO::exists(dbDir)) {
        return Result<void>(MiniSQLException(ErrorCode::STORAGE_DATABASE_EXISTS, "Database already exists: " + dbName));
    }

    if (!FileIO::createDirectory(dbDir)) {
        return Result<void>(MiniSQLException(ErrorCode::STORAGE_FILE_IO_ERROR, "Failed to create database directory"));
    }

    LOG_INFO("TableManager", "Created database: " + dbName);
    return Result<void>();
}

Result<void> TableManager::dropDatabase(const std::string& dbName) {
    std::string dbDir = FileIO::getDatabaseDir(dbName);
    if (!FileIO::exists(dbDir)) {
        return Result<void>(MiniSQLException(ErrorCode::STORAGE_DATABASE_NOT_FOUND, "Database not found: " + dbName));
    }

    if (!FileIO::removeDirectory(dbDir)) {
        return Result<void>(MiniSQLException(ErrorCode::STORAGE_FILE_IO_ERROR, "Failed to drop database"));
    }

    LOG_INFO("TableManager", "Dropped database: " + dbName);
    return Result<void>();
}

Result<std::vector<std::string>> TableManager::listDatabases() {
    std::string dataDir = FileIO::getDataDir();
    if (!FileIO::exists(dataDir)) {
        std::vector<std::string> empty;
        return Result<std::vector<std::string>>(empty);
    }

    std::vector<std::string> dbs = FileIO::listDirectories(dataDir);
    return Result<std::vector<std::string>>(dbs);
}

bool TableManager::databaseExists(const std::string& dbName) {
    std::string dbDir = FileIO::getDatabaseDir(dbName);
    return FileIO::exists(dbDir) && FileIO::isDirectory(dbDir);
}

Result<void> TableManager::createTable(const std::string& dbName, const TableDef& tableDef) {
    if (!databaseExists(dbName)) {
        return Result<void>(MiniSQLException(ErrorCode::STORAGE_DATABASE_NOT_FOUND, "Database not found: " + dbName));
    }

    auto existsResult = tableExists(dbName, tableDef.name);
    if (existsResult.isError()) return Result<void>(existsResult.getError());
    if (*existsResult.getValue()) {
        return Result<void>(MiniSQLException(ErrorCode::STORAGE_TABLE_EXISTS, "Table already exists: " + tableDef.name));
    }

    // 保存表结构
    std::string metaPath = getMetaPath(dbName, tableDef.name);
    std::string jsonStr = tableDefToJson(tableDef);
    if (!FileIO::writeToFile(metaPath, jsonStr)) {
        return Result<void>(MiniSQLException(ErrorCode::STORAGE_FILE_IO_ERROR, "Failed to create table meta file"));
    }

    // 创建空数据文件
    std::string dataPath = getDataPath(dbName, tableDef.name);
    if (!FileIO::writeToFile(dataPath, "")) {
        return Result<void>(MiniSQLException(ErrorCode::STORAGE_FILE_IO_ERROR, "Failed to create table data file"));
    }

    // 初始化行 ID
    if (!saveNextRowId(dbName, tableDef.name, 1).isSuccess()) {
        return Result<void>(MiniSQLException(ErrorCode::STORAGE_FILE_IO_ERROR, "Failed to initialize row ID"));
    }

    LOG_INFO("TableManager", "Created table: " + dbName + "." + tableDef.name);
    return Result<void>();
}

Result<void> TableManager::dropTable(const std::string& dbName, const std::string& tableName) {
    if (!FileIO::removeFile(getMetaPath(dbName, tableName))) {
        return Result<void>(MiniSQLException(ErrorCode::STORAGE_TABLE_NOT_FOUND, "Table not found"));
    }
    FileIO::removeFile(getDataPath(dbName, tableName));
    FileIO::removeFile(getRowIdPath(dbName, tableName));

    LOG_INFO("TableManager", "Dropped table: " + dbName + "." + tableName);
    return Result<void>();
}

Result<bool> TableManager::tableExists(const std::string& dbName, const std::string& tableName) {
    if (!databaseExists(dbName)) {
        return Result<bool>(MiniSQLException(ErrorCode::STORAGE_DATABASE_NOT_FOUND, "Database not found"));
    }

    std::string metaPath = getMetaPath(dbName, tableName);
    return Result<bool>(FileIO::existsFile(metaPath));
}

Result<std::vector<std::string>> TableManager::listTables(const std::string& dbName) {
    if (!databaseExists(dbName)) {
        return Result<std::vector<std::string>>(MiniSQLException(ErrorCode::STORAGE_DATABASE_NOT_FOUND, "Database not found"));
    }

    std::string dbDir = FileIO::getDatabaseDir(dbName);
    std::vector<std::string> tables = FileIO::listFiles(dbDir, ".meta");
    return Result<std::vector<std::string>>(tables);
}

Result<TableDef> TableManager::getTableDef(const std::string& dbName, const std::string& tableName) {
    std::string metaPath = getMetaPath(dbName, tableName);
    if (!FileIO::existsFile(metaPath)) {
        return Result<TableDef>(MiniSQLException(ErrorCode::STORAGE_TABLE_NOT_FOUND, "Table not found"));
    }

    std::string jsonStr = FileIO::readFromFile(metaPath);
    return jsonToTableDef(jsonStr);
}

Result<void> TableManager::insertRow(const std::string& dbName, const std::string& tableName, const Row& row) {
    std::string dataPath = getDataPath(dbName, tableName);
    std::string csv = rowToCsv(row);

    if (!FileIO::appendToFile(dataPath, csv + "\n")) {
        return Result<void>(MiniSQLException(ErrorCode::STORAGE_FILE_IO_ERROR, "Failed to insert row"));
    }

    return Result<void>();
}

Result<TableData> TableManager::loadTable(const std::string& dbName, const std::string& tableName) {
    auto tableDefResult = getTableDef(dbName, tableName);
    if (tableDefResult.isError()) {
        return Result<TableData>(tableDefResult.getError());
    }
    TableDef tableDef = *tableDefResult.getValue();

    std::string dataPath = getDataPath(dbName, tableName);
    if (!FileIO::existsFile(dataPath)) {
        TableData emptyData;
        return Result<TableData>(new TableData(emptyData));
    }

    std::string content = FileIO::readFromFile(dataPath);
    TableData data;

    std::istringstream stream(content);
    std::string line;
    int rowId = 1;
    while (std::getline(stream, line)) {
        if (line.empty()) continue;

        auto rowResult = csvToRow(line, tableDef);
        if (rowResult.isSuccess()) {
            data[rowId++] = *rowResult.getValue();
        }
    }

    return Result<TableData>(new TableData(data));
}

Result<void> TableManager::deleteRow(const std::string& dbName, const std::string& tableName, int rowId) {
    auto tableDataResult = loadTable(dbName, tableName);
    if (tableDataResult.isError()) return Result<void>(tableDataResult.getError());

    TableData data = *tableDataResult.getValue();
    data.erase(rowId);

    // 重写整个文件（简单实现，后续优化）
    std::string dataPath = getDataPath(dbName, tableName);
    std::string content;
    for (const auto& pair : data) {
        content += rowToCsv(pair.second) + "\n";
    }

    if (!FileIO::writeToFile(dataPath, content)) {
        return Result<void>(MiniSQLException(ErrorCode::STORAGE_FILE_IO_ERROR, "Failed to update data file"));
    }

    return Result<void>();
}

Result<void> TableManager::updateRow(const std::string& dbName, const std::string& tableName, int rowId, const Row& row) {
    auto tableDataResult = loadTable(dbName, tableName);
    if (tableDataResult.isError()) return Result<void>(tableDataResult.getError());

    TableData data = *tableDataResult.getValue();
    data[rowId] = row;

    // 重写整个文件
    std::string dataPath = getDataPath(dbName, tableName);
    std::string content;
    for (const auto& pair : data) {
        content += rowToCsv(pair.second) + "\n";
    }

    if (!FileIO::writeToFile(dataPath, content)) {
        return Result<void>(MiniSQLException(ErrorCode::STORAGE_FILE_IO_ERROR, "Failed to update data file"));
    }

    return Result<void>();
}

Result<int> TableManager::getNextRowId(const std::string& dbName, const std::string& tableName) {
    auto result = loadNextRowId(dbName, tableName);
    if (result.isError()) return result;

    int nextId = *result.getValue();
    saveNextRowId(dbName, tableName, nextId + 1);
    return Result<int>(nextId);
}

// ==================== ALTER TABLE 操作 ====================

Result<void> TableManager::addColumn(const std::string& dbName, const std::string& tableName, const ColumnDef& column) {
    // 1. 获取表定义
    auto tableDefResult = getTableDef(dbName, tableName);
    if (tableDefResult.isError()) {
        return Result<void>(tableDefResult.getError());
    }
    TableDef tableDef = *tableDefResult.getValue();

    // 2. 检查列是否已存在
    for (const auto& col : tableDef.columns) {
        if (col.name == column.name) {
            return Result<void>(MiniSQLException(ErrorCode::STORAGE_TABLE_EXISTS, "Column already exists: " + column.name));
        }
    }

    // 3. 添加新列
    tableDef.columns.push_back(column);

    // 4. 保存更新后的表定义
    std::string metaPath = getMetaPath(dbName, tableName);
    std::string jsonStr = tableDefToJson(tableDef);
    if (!FileIO::writeToFile(metaPath, jsonStr)) {
        return Result<void>(MiniSQLException(ErrorCode::STORAGE_FILE_IO_ERROR, "Failed to update table meta"));
    }

    // 5. 为现有数据行添加默认值
    std::string dataPath = getDataPath(dbName, tableName);
    if (FileIO::existsFile(dataPath)) {
        std::string content = FileIO::readFromFile(dataPath);
        if (!content.empty()) {
            std::string defaultVal = column.hasDefault ? column.defaultValue : "NULL";
            std::string newContent;
            std::istringstream stream(content);
            std::string line;
            bool first = true;
            while (std::getline(stream, line)) {
                if (line.empty()) continue;
                if (!first) newContent += "\n";
                newContent += line + "," + defaultVal;
                first = false;
            }
            FileIO::writeToFile(dataPath, newContent);
        }
    }

    LOG_INFO("TableManager", "Added column " + column.name + " to table " + tableName);
    return Result<void>();
}

Result<void> TableManager::dropColumn(const std::string& dbName, const std::string& tableName, const std::string& columnName) {
    // 1. 获取表定义
    auto tableDefResult = getTableDef(dbName, tableName);
    if (tableDefResult.isError()) {
        return Result<void>(tableDefResult.getError());
    }
    TableDef tableDef = *tableDefResult.getValue();

    // 2. 找到列索引
    int colIndex = -1;
    for (size_t i = 0; i < tableDef.columns.size(); ++i) {
        if (tableDef.columns[i].name == columnName) {
            colIndex = i;
            break;
        }
    }

    if (colIndex < 0) {
        return Result<void>(MiniSQLException(ErrorCode::EXEC_COLUMN_NOT_FOUND, "Column not found: " + columnName));
    }

    // 3. 不能删除主键列
    if (tableDef.columns[colIndex].primaryKey) {
        return Result<void>(MiniSQLException(ErrorCode::EXEC_INVALID_VALUE, "Cannot drop primary key column"));
    }

    // 4. 移除列
    tableDef.columns.erase(tableDef.columns.begin() + colIndex);

    // 5. 保存更新后的表定义
    std::string metaPath = getMetaPath(dbName, tableName);
    std::string jsonStr = tableDefToJson(tableDef);
    if (!FileIO::writeToFile(metaPath, jsonStr)) {
        return Result<void>(MiniSQLException(ErrorCode::STORAGE_FILE_IO_ERROR, "Failed to update table meta"));
    }

    // 6. 从数据文件中移除该列
    std::string dataPath = getDataPath(dbName, tableName);
    if (FileIO::existsFile(dataPath)) {
        std::string content = FileIO::readFromFile(dataPath);
        if (!content.empty()) {
            std::string newContent;
            std::istringstream stream(content);
            std::string line;
            bool firstLine = true;
            while (std::getline(stream, line)) {
                if (line.empty()) continue;

                std::istringstream lineStream(line);
                std::string cell;
                std::vector<std::string> cells;
                while (std::getline(lineStream, cell, ',')) {
                    cells.push_back(cell);
                }

                // 移除对应列
                if ((int)cells.size() > colIndex) {
                    cells.erase(cells.begin() + colIndex);
                }

                // 重新拼接
                std::string newLine;
                for (size_t i = 0; i < cells.size(); ++i) {
                    if (i > 0) newLine += ",";
                    newLine += cells[i];
                }

                if (!firstLine) newContent += "\n";
                newContent += newLine;
                firstLine = false;
            }
            FileIO::writeToFile(dataPath, newContent);
        }
    }

    LOG_INFO("TableManager", "Dropped column " + columnName + " from table " + tableName);
    return Result<void>();
}

Result<void> TableManager::renameTable(const std::string& dbName, const std::string& oldName, const std::string& newName) {
    // 1. 获取旧表定义
    auto tableDefResult = getTableDef(dbName, oldName);
    if (tableDefResult.isError()) {
        return Result<void>(tableDefResult.getError());
    }
    TableDef tableDef = *tableDefResult.getValue();

    // 2. 检查新表名是否已存在
    auto existsResult = tableExists(dbName, newName);
    if (existsResult.isError()) return Result<void>(existsResult.getError());
    if (*existsResult.getValue()) {
        return Result<void>(MiniSQLException(ErrorCode::STORAGE_TABLE_EXISTS, "Table already exists: " + newName));
    }

    // 3. 更新表定义
    tableDef.name = newName;

    // 4. 删除旧文件
    std::string oldMetaPath = getMetaPath(dbName, oldName);
    std::string oldDataPath = getDataPath(dbName, oldName);
    std::string oldRowIdPath = getRowIdPath(dbName, oldName);

    // 5. 创建新文件
    std::string newMetaPath = getMetaPath(dbName, newName);
    std::string newDataPath = getDataPath(dbName, newName);
    std::string newRowIdPath = getRowIdPath(dbName, newName);

    std::string jsonStr = tableDefToJson(tableDef);
    if (!FileIO::writeToFile(newMetaPath, jsonStr)) {
        return Result<void>(MiniSQLException(ErrorCode::STORAGE_FILE_IO_ERROR, "Failed to create new table meta"));
    }

    FileIO::renameFile(oldDataPath, newDataPath);
    FileIO::renameFile(oldRowIdPath, newRowIdPath);
    FileIO::removeFile(oldMetaPath);

    LOG_INFO("TableManager", "Renamed table " + oldName + " to " + newName);
    return Result<void>();
}

// ==================== 私有方法 ====================

std::string TableManager::getMetaPath(const std::string& dbName, const std::string& tableName) {
    return FileIO::join(FileIO::getDatabaseDir(dbName), tableName + ".meta");
}

std::string TableManager::getDataPath(const std::string& dbName, const std::string& tableName) {
    return FileIO::join(FileIO::getDatabaseDir(dbName), tableName + ".data");
}

std::string TableManager::getRowIdPath(const std::string& dbName, const std::string& tableName) {
    return FileIO::join(FileIO::getDatabaseDir(dbName), tableName + ".rowid");
}

std::string TableManager::tableDefToJson(const TableDef& tableDef) {
    json j;
    j["table_name"] = tableDef.name;
    j["database"] = tableDef.database;
    j["engine"] = tableDef.engine;
    j["comment"] = tableDef.comment;

    // 序列化列
    json columns = json::array();
    for (const auto& col : tableDef.columns) {
        json c;
        c["name"] = col.name;
        c["type"] = TypeUtils::typeToString(col.type);
        c["length"] = col.length;
        c["precision"] = col.precision;
        c["scale"] = col.scale;
        c["not_null"] = col.notNull;
        c["primary_key"] = col.primaryKey;
        c["unique"] = col.unique;
        c["auto_increment"] = col.autoIncrement;
        c["has_default"] = col.hasDefault;
        c["default_value"] = col.defaultValue;
        c["comment"] = col.comment;
        columns.push_back(c);
    }
    j["columns"] = columns;

    // 序列化索引
    json indexes = json::array();
    for (const auto& idx : tableDef.indexes) {
        json ix;
        ix["name"] = idx.name;
        ix["table_name"] = idx.tableName;
        ix["columns"] = idx.columns;
        ix["type"] = (idx.type == IndexType::BTREE ? "BTREE" : "HASH");
        ix["unique"] = idx.unique;
        indexes.push_back(ix);
    }
    j["indexes"] = indexes;

    // 序列化外键
    json foreignKeys = json::array();
    for (const auto& fk : tableDef.foreignKeys) {
        json f;
        f["name"] = fk.name;
        f["column"] = fk.column;
        f["ref_table"] = fk.refTable;
        f["ref_column"] = fk.refColumn;
        f["on_delete"] = fk.onDelete;
        f["on_update"] = fk.onUpdate;
        foreignKeys.push_back(f);
    }
    j["foreign_keys"] = foreignKeys;

    return j.dump(4);
}

Result<TableDef> TableManager::jsonToTableDef(const std::string& jsonStr) {
    try {
        json j = json::parse(jsonStr);
        TableDef def;

        def.name = j["table_name"];
        def.database = j.value("database", "");
        def.engine = j.value("engine", "MiniSQL");
        def.comment = j.value("comment", "");

        // 反序列化列
        for (const auto& colJson : j["columns"]) {
            ColumnDef col;
            col.name = colJson["name"];
            col.type = TypeUtils::stringToType(colJson["type"]);
            col.length = colJson.value("length", 0);
            col.precision = colJson.value("precision", 0);
            col.scale = colJson.value("scale", 0);
            col.notNull = colJson.value("not_null", false);
            col.primaryKey = colJson.value("primary_key", false);
            col.unique = colJson.value("unique", false);
            col.autoIncrement = colJson.value("auto_increment", false);
            col.hasDefault = colJson.value("has_default", false);
            col.defaultValue = colJson.value("default_value", "");
            col.comment = colJson.value("comment", "");
            def.columns.push_back(col);
        }

        // 反序列化索引
        if (j.contains("indexes")) {
            for (const auto& idxJson : j["indexes"]) {
                IndexDef idx;
                idx.name = idxJson["name"];
                idx.tableName = idxJson.value("table_name", def.name);
                idx.columns = idxJson["columns"].get<std::vector<std::string>>();
                std::string idxType = idxJson.value("type", "BTREE");
                idx.type = (idxType == "HASH" ? IndexType::HASH : IndexType::BTREE);
                idx.unique = idxJson.value("unique", false);
                def.indexes.push_back(idx);
            }
        }

        // 反序列化外键
        if (j.contains("foreign_keys")) {
            for (const auto& fkJson : j["foreign_keys"]) {
                ForeignKeyDef fk;
                fk.name = fkJson["name"];
                fk.column = fkJson["column"];
                fk.refTable = fkJson["ref_table"];
                fk.refColumn = fkJson["ref_column"];
                fk.onDelete = fkJson.value("on_delete", "NO ACTION");
                fk.onUpdate = fkJson.value("on_update", "NO ACTION");
                def.foreignKeys.push_back(fk);
            }
        }

        return Result<TableDef>(def);
    } catch (const std::exception& e) {
        return Result<TableDef>(MiniSQLException(ErrorCode::STORAGE_FILE_CORRUPTED, std::string("Failed to parse table meta: ") + e.what()));
    }
}

std::string TableManager::rowToCsv(const Row& row) {
    std::stringstream ss;
    for (size_t i = 0; i < row.size(); i++) {
        if (i > 0) ss << ",";
        ss << row[i].toSQLString();
    }
    return ss.str();
}

Result<Row> TableManager::csvToRow(const std::string& csv, const TableDef& tableDef) {
    Row row;
    std::stringstream ss(csv);
    std::string cell;

    size_t colIndex = 0;
    while (std::getline(ss, cell, ',')) {
        if (colIndex >= tableDef.columns.size()) {
            return Result<Row>(MiniSQLException(ErrorCode::STORAGE_FILE_CORRUPTED, "Too many columns in row"));
        }

        // 去除引号（简单处理）
        if (!cell.empty() && cell[0] == '\'' && cell.back() == '\'') {
            cell = cell.substr(1, cell.size() - 2);
            // 处理转义的撇号
            std::string unescaped;
            for (size_t i = 0; i < cell.size(); ++i) {
                if (cell[i] == '\'' && i + 1 < cell.size() && cell[i + 1] == '\'') {
                    unescaped += '\'';
                    ++i;
                } else {
                    unescaped += cell[i];
                }
            }
            cell = unescaped;
        }

        Value val = Value::parseFrom(cell, tableDef.columns[colIndex].type);
        row.push_back(val);
        colIndex++;
    }

    return Result<Row>(row);
}

Result<int> TableManager::loadNextRowId(const std::string& dbName, const std::string& tableName) {
    std::string rowIdPath = getRowIdPath(dbName, tableName);
    if (!FileIO::existsFile(rowIdPath)) {
        return Result<int>(1);  // 默认从 1 开始
    }

    std::string content = FileIO::readFromFile(rowIdPath);
    try {
        int id = std::stoi(content);
        return Result<int>(id);
    } catch (...) {
        return Result<int>(MiniSQLException(ErrorCode::STORAGE_FILE_CORRUPTED, "Invalid row ID file"));
    }
}

Result<void> TableManager::saveNextRowId(const std::string& dbName, const std::string& tableName, int rowId) {
    std::string rowIdPath = getRowIdPath(dbName, tableName);
    if (!FileIO::writeToFile(rowIdPath, std::to_string(rowId))) {
        return Result<void>(MiniSQLException(ErrorCode::STORAGE_FILE_IO_ERROR, "Failed to save row ID"));
    }
    return Result<void>();
}
