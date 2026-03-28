// src/server/transaction/WALManager.h
#pragma once

#include <string>
#include <fstream>
#include <mutex>
#include <vector>
#include <cstdint>

namespace minisql {

enum class LogType { BEGIN, INSERT, UPDATE, DELETE, COMMIT };

struct WALEntry {
    uint64_t txnId;
    LogType type;
    std::string table;
    std::string key;
    std::string value;
    uint64_t lsn;
};

class WALManager {
public:
    static WALManager& getInstance();

    // 初始化WAL
    void init(const std::string& dataDir);

    // 写日志
    void writeLog(uint64_t txnId, LogType type,
                  const std::string& table = "",
                  const std::string& key = "",
                  const std::string& value = "");

    // 刷新到磁盘
    void flush();

    // 崩溃恢复
    std::vector<WALEntry> recover();

    // 清理WAL文件
    void truncate();

    // 获取WAL文件路径
    std::string getWALPath() const { return walPath_; }

private:
    WALManager() = default;
    ~WALManager();

    WALManager(const WALManager&) = delete;
    WALManager& operator=(const WALManager&) = delete;

    uint64_t getNextLSN();

    std::string walPath_;
    std::ofstream walFile_;
    std::mutex mutex_;
    uint64_t currentLSN_ = 0;
};

}  // namespace minisql
