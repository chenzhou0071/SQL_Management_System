// src/server/transaction/Transaction.cpp
#include "Transaction.h"

namespace minisql {

Transaction::Transaction(uint64_t txnId) : txnId_(txnId) {}

void Transaction::recordOperation(OpType type, const std::string& table,
                                 const std::string& key,
                                 const std::string& oldValue,
                                 const std::string& newValue) {
    operations_.push_back({type, table, key, oldValue, newValue});
}

}  // namespace minisql
