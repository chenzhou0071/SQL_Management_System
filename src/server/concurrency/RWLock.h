// src/server/concurrency/RWLock.h
#pragma once

#include <shared_mutex>

namespace minisql {

class RWLock {
public:
    // 获取读锁(共享锁)
    void lock_shared() { mutex_.lock_shared(); }
    void unlock_shared() { mutex_.unlock_shared(); }

    // 获取写锁(独占锁)
    void lock() { mutex_.lock(); }
    void unlock() { mutex_.unlock(); }

    // RAII 读锁 guard
    class ReadGuard {
    public:
        explicit ReadGuard(RWLock& lock) : lock_(lock) { lock_.lock_shared(); }
        ~ReadGuard() { lock_.unlock_shared(); }
        ReadGuard(const ReadGuard&) = delete;
        ReadGuard& operator=(const ReadGuard&) = delete;
    private:
        RWLock& lock_;
    };

    // RAII 写锁 guard
    class WriteGuard {
    public:
        explicit WriteGuard(RWLock& lock) : lock_(lock) { lock_.lock(); }
        ~WriteGuard() { lock_.unlock(); }
        WriteGuard(const WriteGuard&) = delete;
        WriteGuard& operator=(const WriteGuard&) = delete;
    private:
        RWLock& lock_;
    };

private:
    std::shared_mutex mutex_;
};

}  // namespace minisql
