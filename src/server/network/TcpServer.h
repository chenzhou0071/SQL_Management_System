// src/server/network/TcpServer.h
#pragma once

#include <memory>
#include <string>

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
    int port_;
    int threadCount_;
    std::string dataDir_;
    std::unique_ptr<Reactor> reactor_;
};

}  // namespace minisql
