// src/server/transaction/TransactionManager.h
#pragma once

#include "Transaction.h"
#include "WALManager.h"
#include <atomic>
#include <mutex>
#include <unordered_map>
#include <thread>

namespace minisql {

class TransactionManager {
public:
    static TransactionManager& getInstance();

    // 获取当前线程的事务
    Transaction* getCurrentTransaction();

    // 开始事务
    Transaction* begin();

    // 提交事务
    bool commit();

    // 回滚事务
    bool rollback();

    // 初始化
    void init(const std::string& dataDir);

    // 崩溃恢复
    void recover();

private:
    TransactionManager() = default;

    void removeTransaction(uint64_t txnId);
    void addTransaction(uint64_t txnId, Transaction* txn);

    thread_local static Transaction* currentTransaction_;

    std::atomic<uint64_t> nextTxnId_{1};
    std::mutex txnMapMutex_;
    std::unordered_map<uint64_t, Transaction*> activeTransactions_;

    WALManager& wal_ = WALManager::getInstance();
};

}  // namespace minisql
