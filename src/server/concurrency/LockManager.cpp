// src/server/concurrency/LockManager.cpp
#include "LockManager.h"
#include <chrono>
#include <thread>

namespace minisql {

LockManager& LockManager::getInstance() {
    static LockManager instance;
    return instance;
}

RWLock& LockManager::getOrCreateTableLock(const std::string& table) {
    std::lock_guard<std::mutex> lock(tableMutex_);
    auto it = tableLocks_.find(table);
    if (it == tableLocks_.end()) {
        auto result = tableLocks_.emplace(table, std::make_unique<RWLock>());
        return *result.first->second;
    }
    return *it->second;
}

RWLock& LockManager::getOrCreateIndexLock(const std::string& index) {
    std::lock_guard<std::mutex> lock(indexMutex_);
    auto it = indexLocks_.find(index);
    if (it == indexLocks_.end()) {
        auto result = indexLocks_.emplace(index, std::make_unique<RWLock>());
        return *result.first->second;
    }
    return *it->second;
}

std::mutex& LockManager::getOrCreateFileMutex(const std::string& filePath) {
    std::lock_guard<std::mutex> lock(fileMutex_);
    auto it = fileMutexes_.find(filePath);
    if (it == fileMutexes_.end()) {
        auto result = fileMutexes_.emplace(filePath, std::make_unique<std::mutex>());
        return *result.first->second;
    }
    return *it->second;
}

bool LockManager::acquireTableLock(const std::string& table, LockType type) {
    RWLock& lock = getOrCreateTableLock(table);
    auto start = std::chrono::steady_clock::now();

    while (true) {
        if (type == LockType::READ) {
            lock.lock_shared();
        } else {
            lock.lock();
        }

        auto now = std::chrono::steady_clock::now();
        if (now - start > std::chrono::seconds(timeoutSeconds_)) {
            // 超时,释放已获取的锁
            if (type == LockType::READ) {
                lock.unlock_shared();
            } else {
                lock.unlock();
            }
            return false;
        }
        return true;
    }
}

void LockManager::releaseTableLock(const std::string& table) {
    // 表锁的生命周期由 LockManager 管理,这里不需要主动释放
    // 实际使用中使用 ReadGuard/WriteGuard 自动管理
}

bool LockManager::acquireIndexLock(const std::string& index, LockType type) {
    RWLock& lock = getOrCreateIndexLock(index);
    auto start = std::chrono::steady_clock::now();

    while (true) {
        if (type == LockType::READ) {
            lock.lock_shared();
        } else {
            lock.lock();
        }

        auto now = std::chrono::steady_clock::now();
        if (now - start > std::chrono::seconds(timeoutSeconds_)) {
            if (type == LockType::READ) {
                lock.unlock_shared();
            } else {
                lock.unlock();
            }
            return false;
        }
        return true;
    }
}

void LockManager::releaseIndexLock(const std::string& index) {
    // 索引锁由 LockManager 管理生命周期
}

void LockManager::acquireFileLock(const std::string& filePath) {
    std::mutex& mutex = getOrCreateFileMutex(filePath);
    mutex.lock();
}

void LockManager::releaseFileLock(const std::string& filePath) {
    std::mutex& mutex = getOrCreateFileMutex(filePath);
    mutex.unlock();
}

}  // namespace minisql
