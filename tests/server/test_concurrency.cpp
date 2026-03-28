// tests/server/test_concurrency.cpp
#include "../../src/server/concurrency/LockManager.h"
#include <thread>
#include <vector>
#include <iostream>
#include <cassert>

using namespace minisql;

void testTableLock() {
    LockManager& mgr = LockManager::getInstance();

    // 测试获取读锁
    assert(mgr.acquireTableLock("users", LockType::READ));
    std::cout << "[PASS] Acquire read lock" << std::endl;

    // 测试再次获取读锁(应该成功)
    assert(mgr.acquireTableLock("users", LockType::READ));
    std::cout << "[PASS] Acquire second read lock" << std::endl;

    mgr.releaseTableLock("users");
    mgr.releaseTableLock("users");
}

void testWriteLock() {
    LockManager& mgr = LockManager::getInstance();

    // 获取写锁
    assert(mgr.acquireTableLock("orders", LockType::WRITE));
    std::cout << "[PASS] Acquire write lock" << std::endl;

    mgr.releaseTableLock("orders");
}

void testMultiThreadRead() {
    LockManager& mgr = LockManager::getInstance();
    int counter = 0;

    std::vector<std::thread> threads;
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([&]() {
            mgr.acquireTableLock("test_table", LockType::READ);
            counter++;
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            mgr.releaseTableLock("test_table");
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    assert(counter == 10);
    std::cout << "[PASS] Multi-thread read lock: " << counter << " threads completed" << std::endl;
}

void testFileLock() {
    LockManager& mgr = LockManager::getInstance();

    mgr.acquireFileLock("/tmp/test_file.txt");
    std::cout << "[PASS] Acquire file lock" << std::endl;
    mgr.releaseFileLock("/tmp/test_file.txt");
}

int main() {
    std::cout << "=== Concurrency Test ===" << std::endl;

    testTableLock();
    testWriteLock();
    testMultiThreadRead();
    testFileLock();

    std::cout << "=== All Tests Passed ===" << std::endl;
    return 0;
}
