// src/server/transaction/TransactionManager.cpp
#include "TransactionManager.h"
#include "../../common/Logger.h"

namespace minisql {

thread_local Transaction* TransactionManager::currentTransaction_ = nullptr;

TransactionManager& TransactionManager::getInstance() {
    static TransactionManager instance;
    return instance;
}

void TransactionManager::init(const std::string& dataDir) {
    wal_.init(dataDir);
    recover();
}

Transaction* TransactionManager::begin() {
    uint64_t txnId = nextTxnId_++;

    auto* txn = new Transaction(txnId);
    txn->setState(Transaction::State::ACTIVE);

    // 记录到活动事务
    addTransaction(txnId, txn);

    // 设为当前线程事务
    currentTransaction_ = txn;

    // 写WAL日志
    wal_.writeLog(txnId, LogType::BEGIN);

    LOG_INFO("TransactionManager", "Transaction " + std::to_string(txnId) + " started");

    return txn;
}

Transaction* TransactionManager::getCurrentTransaction() {
    return currentTransaction_;
}

bool TransactionManager::commit() {
    auto* txn = currentTransaction_;
    if (!txn) {
        LOG_ERROR("TransactionManager", "No active transaction to commit");
        return false;
    }

    uint64_t txnId = txn->getId();

    // 写COMMIT日志
    wal_.writeLog(txnId, LogType::COMMIT);
    wal_.flush();

    txn->setState(Transaction::State::COMMITTED);

    // 从活动事务移除
    removeTransaction(txnId);

    // 清理当前线程事务
    currentTransaction_ = nullptr;

    LOG_INFO("TransactionManager", "Transaction " + std::to_string(txnId) + " committed");

    return true;
}

bool TransactionManager::rollback() {
    auto* txn = currentTransaction_;
    if (!txn) {
        LOG_ERROR("TransactionManager", "No active transaction to rollback");
        return false;
    }

    uint64_t txnId = txn->getId();

    // 从后往前撤销操作
    for (auto it = txn->getOperations().rbegin();
         it != txn->getOperations().rend(); ++it) {
        const auto& op = *it;
        LOG_INFO("TransactionManager",
                 "Rolling back: " + std::to_string(static_cast<int>(op.type)) +
                 " on " + op.table);
        // TODO: 调用存储引擎执行撤销
    }

    txn->setState(Transaction::State::ABORTED);

    // 从活动事务移除
    removeTransaction(txnId);

    // 清理当前线程事务
    currentTransaction_ = nullptr;

    LOG_INFO("TransactionManager", "Transaction " + std::to_string(txnId) + " rolled back");

    return true;
}

void TransactionManager::addTransaction(uint64_t txnId, Transaction* txn) {
    std::lock_guard<std::mutex> lock(txnMapMutex_);
    activeTransactions_[txnId] = txn;
}

void TransactionManager::removeTransaction(uint64_t txnId) {
    std::lock_guard<std::mutex> lock(txnMapMutex_);
    activeTransactions_.erase(txnId);
}

void TransactionManager::recover() {
    auto entries = wal_.recover();

    // 简单恢复策略:重做所有已COMMIT的事务
    // TODO: 实现完整的恢复逻辑

    // 清空WAL
    wal_.truncate();

    LOG_INFO("TransactionManager", "Recovery completed");
}

}  // namespace minisql
