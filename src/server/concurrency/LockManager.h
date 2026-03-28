// src/server/concurrency/LockManager.h
#pragma once

#include "RWLock.h"
#include <string>
#include <map>
#include <mutex>
#include <memory>

namespace minisql {

enum class LockType { READ, WRITE };

class LockManager {
public:
    static LockManager& getInstance();

    // 获取表级锁
    bool acquireTableLock(const std::string& table, LockType type);

    // 释放表级锁
    void releaseTableLock(const std::string& table);

    // 获取索引锁
    bool acquireIndexLock(const std::string& index, LockType type);

    // 释放索引锁
    void releaseIndexLock(const std::string& index);

    // 文件操作锁
    void acquireFileLock(const std::string& filePath);
    void releaseFileLock(const std::string& filePath);

    // 超时配置(秒)
    void setTimeout(int seconds) { timeoutSeconds_ = seconds; }

private:
    LockManager() = default;
    ~LockManager() = default;
    LockManager(const LockManager&) = delete;
    LockManager& operator=(const LockManager&) = delete;

    // 获取或创建表锁
    RWLock& getOrCreateTableLock(const std::string& table);

    // 获取或创建索引锁
    RWLock& getOrCreateIndexLock(const std::string& index);

    // 获取或创建文件锁
    std::mutex& getOrCreateFileMutex(const std::string& filePath);

    int timeoutSeconds_ = 5;

    std::map<std::string, std::unique_ptr<RWLock>> tableLocks_;
    std::map<std::string, std::unique_ptr<RWLock>> indexLocks_;
    std::map<std::string, std::unique_ptr<std::mutex>> fileMutexes_;
    std::mutex tableMutex_;
    std::mutex indexMutex_;
    std::mutex fileMutex_;
};

}  // namespace minisql
