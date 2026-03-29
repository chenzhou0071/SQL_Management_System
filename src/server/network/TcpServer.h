// src/server/network/TcpServer.h
#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <mutex>

#include "Session.h"

namespace minisql {

class Reactor;

class TcpServer {
public:
    TcpServer(int port = 3306, int threadCount = 4);
    ~TcpServer();

    void start();
    void stop();

    void setDataDir(const std::string& dir) { dataDir_ = dir; }

private:
    // 会话管理
    server::Session& getOrCreateSession(int fd);
    void removeSession(int fd);

    int port_;
    int threadCount_;
    std::string dataDir_;
    std::unique_ptr<Reactor> reactor_;

    // 会话存储：每个 fd 对应一个 Session
    std::unordered_map<int, server::Session> sessions_;
    std::mutex sessionsMutex_;
};

}  // namespace minisql
