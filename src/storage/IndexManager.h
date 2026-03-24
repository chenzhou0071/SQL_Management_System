#pragma once

#include "BPlusTree.h"
#include "../common/Error.h"
#include "../common/Type.h"
#include <string>
#include <map>
#include <memory>

namespace minisql {
namespace storage {

// 索引类型（与 Type.h 中的 IndexType 不同，用于索引管理器）
enum class IndexUsageType {
    PRIMARY,
    UNIQUE,
    NORMAL
};

// 索引条目（存储运行时索引信息，包含 B+树指针）
struct IndexEntry {
    std::string name;
    std::string tableName;
    std::string columnName;  // 单列索引
    IndexUsageType usageType;
    std::shared_ptr<BPlusTree> tree;
};

class IndexManager {
public:
    static IndexManager& getInstance();

    // 索引操作
    Result<void> createIndex(const std::string& dbName, const std::string& tableName,
                            const std::string& indexName, const std::string& columnName,
                            IndexUsageType type);
    Result<void> dropIndex(const std::string& dbName, const std::string& indexName);

    // 查找索引
    Result<std::shared_ptr<BPlusTree>> getIndex(const std::string& dbName, const std::string& indexName);
    Result<std::shared_ptr<BPlusTree>> getPrimaryKeyIndex(const std::string& dbName, const std::string& tableName);

    // 索引路径
    std::string getIndexPath(const std::string& dbName, const std::string& indexName);

    // 保存/加载索引
    Result<void> saveIndex(const std::string& dbName, const std::string& indexName);
    Result<void> loadIndex(const std::string& dbName, const std::string& indexName);

private:
    IndexManager() = default;
    ~IndexManager() = default;

    IndexManager(const IndexManager&) = delete;
    IndexManager& operator=(const IndexManager&) = delete;

    // 内存中的索引缓存（数据库名 -> 索引名 -> 索引条目）
    std::map<std::string, std::map<std::string, IndexEntry>> indexes_;
};

}  // namespace storage
}  // namespace minisql
