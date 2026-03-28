// tests/server/test_transaction.cpp
#include "../../src/server/transaction/TransactionManager.h"
#include <iostream>
#include <cassert>
#include <sys/stat.h>

using namespace minisql;

void testBasicTransaction() {
    TransactionManager& mgr = TransactionManager::getInstance();

    // 创建测试目录
    mkdir("/tmp/minisql_test", 0755);
    mgr.init("/tmp/minisql_test");

    // 开始事务
    auto* txn = mgr.begin();
    assert(txn != nullptr);
    assert(txn->getId() == 1);
    assert(txn->getState() == Transaction::State::ACTIVE);

    std::cout << "[PASS] Begin transaction" << std::endl;

    // 提交事务
    bool result = mgr.commit();
    assert(result);
    assert(txn->getState() == Transaction::State::COMMITTED);

    std::cout << "[PASS] Commit transaction" << std::endl;
}

void testRollback() {
    TransactionManager& mgr = TransactionManager::getInstance();

    // 开始事务
    auto* txn = mgr.begin();
    assert(txn != nullptr);

    // 记录操作
    txn->recordOperation(OpType::INSERT, "users", "1", "", "Alice");

    std::cout << "[PASS] Record operation" << std::endl;

    // 回滚事务
    bool result = mgr.rollback();
    assert(result);
    assert(txn->getState() == Transaction::State::ABORTED);

    std::cout << "[PASS] Rollback transaction" << std::endl;
}

void testTransactionIsolation() {
    TransactionManager& mgr = TransactionManager::getInstance();

    // 事务1
    auto* txn1 = mgr.begin();
    assert(txn1->getId() == 3);

    // 事务2
    auto* txn2 = mgr.begin();
    assert(txn2->getId() == 4);

    std::cout << "[PASS] Multiple transactions with unique IDs" << std::endl;

    // 提交
    mgr.commit();
    mgr.commit();
}

int main() {
    std::cout << "=== Transaction Test ===" << std::endl;

    testBasicTransaction();
    testRollback();
    testTransactionIsolation();

    std::cout << "=== All Tests Passed ===" << std::endl;
    return 0;
}
