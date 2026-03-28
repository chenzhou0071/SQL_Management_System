// src/server/network/TcpConnection.h
#pragma once

#include <string>
#include <cstdint>

namespace minisql {

class Transaction;

class TcpConnection {
public:
    TcpConnection(int fd);
    ~TcpConnection();

    int getFd() const { return fd_; }

    // 读取数据
    std::string read();

    // 发送数据
    bool send(const std::string& data);

    // 关闭连接
    void close();

    // 设置当前事务
    void setTransaction(Transaction* txn) { currentTxn_ = txn; }
    Transaction* getTransaction() const { return currentTxn_; }

    // 获取缓冲区中的完整命令
    std::string extractCommand();

private:
    int fd_;
    std::string buffer_;
    Transaction* currentTxn_ = nullptr;
};

}  // namespace minisql
