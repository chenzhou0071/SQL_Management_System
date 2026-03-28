// src/server/network/Reactor.h
#pragma once

#include <sys/epoll.h>
#include <memory>
#include <vector>
#include <functional>
#include <atomic>

namespace minisql {

class TcpConnection;
class ThreadPool;

class Reactor {
public:
    Reactor(int port, int threadCount = 4);
    ~Reactor();

    void start();
    void stop();

    // 处理客户端连接
    void handleNewConnection();

    // 处理读事件
    void handleRead(int fd);

    // 设置SQL处理函数
    void setSqlHandler(std::function<void(int fd, const std::string& sql)> handler) {
        sqlHandler_ = handler;
    }

private:
    void eventLoop();
    int createEpoll();
    int createListenSocket(int port);

    int epollFd_;
    int listenFd_;
    int port_;
    std::unique_ptr<ThreadPool> threadPool_;
    std::vector<std::thread> reactorThreads_;
    std::atomic<bool> running_{false};

    std::function<void(int fd, const std::string& sql)> sqlHandler_;
};

}  // namespace minisql
