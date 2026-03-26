#include "IndexManager.h"
#include "TableManager.h"
#include "FileIO.h"
#include "../common/Logger.h"

using namespace minisql;
using namespace storage;

IndexManager& IndexManager::getInstance() {
    static IndexManager instance;
    return instance;
}

Result<void> IndexManager::createIndex(const std::string& dbName, const std::string& tableName,
                                       const std::string& indexName, const std::string& columnName,
                                       IndexUsageType type) {
    // 检查表是否存在
    auto tableDefResult = TableManager::getInstance().getTableDef(dbName, tableName);
    if (tableDefResult.isError()) {
        return Result<void>(tableDefResult.getError());
    }

    // 创建 B+树
    auto tree = std::make_shared<BPlusTree>(200);  // 阶数 200
    IndexEntry entry;
    entry.name = indexName;
    entry.tableName = tableName;
    entry.columnName = columnName;
    entry.usageType = type;
    entry.tree = tree;

    // 如果是主键索引，加载现有数据
    if (type == IndexUsageType::PRIMARY) {
        auto dataResult = TableManager::getInstance().loadTable(dbName, tableName);
        if (dataResult.isSuccess()) {
            TableData data = *dataResult.getValue();
            for (const auto& pair : data) {
                const Row& row = pair.second;
                if (row.size() > 0) {
                    int64_t key = row[0].getInt();  // 假设第一列是主键
                    tree->insert(key, pair.first);
                }
            }
        }
    }

    // 保存到内存
    indexes_[dbName][indexName] = entry;

    // 持久化
    return saveIndex(dbName, indexName);
}

Result<void> IndexManager::dropIndex(const std::string& dbName, const std::string& indexName) {
    if (indexes_[dbName].find(indexName) == indexes_[dbName].end()) {
        return Result<void>(MiniSQLException(ErrorCode::INDEX_NOT_FOUND, "Index not found"));
    }

    indexes_[dbName].erase(indexName);
    FileIO::removeFile(getIndexPath(dbName, indexName));

    LOG_INFO("IndexManager", "Dropped index: " + indexName);
    return Result<void>();
}

Result<std::shared_ptr<BPlusTree>> IndexManager::getIndex(const std::string& dbName, const std::string& indexName) {
    if (indexes_[dbName].find(indexName) == indexes_[dbName].end()) {
        return Result<std::shared_ptr<BPlusTree>>(MiniSQLException(ErrorCode::INDEX_NOT_FOUND, "Index not found"));
    }
    return Result<std::shared_ptr<BPlusTree>>(indexes_[dbName][indexName].tree);
}

Result<std::shared_ptr<BPlusTree>> IndexManager::getPrimaryKeyIndex(const std::string& dbName, const std::string& tableName) {
    for (const auto& pair : indexes_[dbName]) {
        if (pair.second.tableName == tableName && pair.second.usageType == IndexUsageType::PRIMARY) {
            return Result<std::shared_ptr<BPlusTree>>(pair.second.tree);
        }
    }
    return Result<std::shared_ptr<BPlusTree>>(MiniSQLException(ErrorCode::INDEX_NOT_FOUND, "Primary key index not found"));
}

std::string IndexManager::getIndexPath(const std::string& dbName, const std::string& indexName) {
    return FileIO::join(FileIO::getDatabaseDir(dbName), indexName + ".idx");
}

Result<void> IndexManager::saveIndex(const std::string& dbName, const std::string& indexName) {
    if (indexes_[dbName].find(indexName) == indexes_[dbName].end()) {
        return Result<void>(MiniSQLException(ErrorCode::INDEX_NOT_FOUND, "Index not found"));
    }

    auto& entry = indexes_[dbName][indexName];

    // 序列化 B+树
    std::string data = entry.tree->serialize();

    std::string indexPath = getIndexPath(dbName, indexName);
    if (!FileIO::writeToFile(indexPath, data)) {
        return Result<void>(MiniSQLException(ErrorCode::STORAGE_FILE_IO_ERROR, "Failed to save index"));
    }

    LOG_INFO("IndexManager", "Saved index: " + indexName +
             " (" + std::to_string(entry.tree->getNodeCount()) + " nodes)");

    return Result<void>();
}

Result<void> IndexManager::loadIndex(const std::string& dbName, const std::string& indexName) {
    std::string indexPath = getIndexPath(dbName, indexName);

    // 检查文件是否存在
    if (!FileIO::existsFile(indexPath)) {
        return Result<void>(MiniSQLException(ErrorCode::INDEX_NOT_FOUND, "Index file not found"));
    }

    // 读取索引数据
    std::string data = FileIO::readFromFile(indexPath);
    if (data.empty() && !FileIO::existsFile(indexPath)) {
        return Result<void>(MiniSQLException(ErrorCode::STORAGE_FILE_IO_ERROR, "Failed to read index"));
    }

    // 反序列化 B+树
    auto tree = std::make_shared<BPlusTree>();
    tree->deserialize(data);

    LOG_INFO("IndexManager", "Loaded index: " + indexName +
             " (" + std::to_string(tree->getNodeCount()) + " nodes)");

    // 更新内存缓存
    if (indexes_[dbName].find(indexName) == indexes_[dbName].end()) {
        // 创建新的索引条目（元数据需要从 .meta 文件加载）
        IndexEntry entry;
        entry.name = indexName;
        entry.tree = tree;
        indexes_[dbName][indexName] = entry;
    } else {
        // 更新现有索引条目
        indexes_[dbName][indexName].tree = tree;
    }

    return Result<void>();
}
