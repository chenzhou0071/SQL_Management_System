// src/server/transaction/Transaction.h
#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <memory>

namespace minisql {

enum class OpType { INSERT, UPDATE, DELETE };

struct Operation {
    OpType type;
    std::string table;
    std::string key;
    std::string oldValue;
    std::string newValue;
};

class Transaction {
public:
    enum class State { ACTIVE, COMMITTED, ABORTED };

    explicit Transaction(uint64_t txnId);

    uint64_t getId() const { return txnId_; }
    State getState() const { return state_; }

    void recordOperation(OpType type, const std::string& table,
                        const std::string& key,
                        const std::string& oldValue = "",
                        const std::string& newValue = "");

    const std::vector<Operation>& getOperations() const { return operations_; }

    void setState(State state) { state_ = state; }

private:
    uint64_t txnId_;
    State state_ = State::ACTIVE;
    std::vector<Operation> operations_;
};

}  // namespace minisql
