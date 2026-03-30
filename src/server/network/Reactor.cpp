// src/server/network/Reactor.cpp
#include "Reactor.h"
#include "TcpConnection.h"
#include "ThreadPool.h"
#include "SqlProtocol.h"
#include "../../common/Logger.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <errno.h>

namespace minisql {

Reactor::Reactor(int port, int threadCount)
    : port_(port), threadPool_(std::make_unique<ThreadPool>(threadCount)) {
    listenFd_ = createListenSocket(port);
    epollFd_ = createEpoll();
}

Reactor::~Reactor() {
    stop();
    if (epollFd_ >= 0) close(epollFd_);
    if (listenFd_ >= 0) close(listenFd_);
}

int Reactor::createListenSocket(int port) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        LOG_ERROR("Reactor", "Failed to create socket");
        return -1;
    }

    // 设置SO_REUSEADDR
    int opt = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        LOG_ERROR("Reactor", "Failed to bind to port " + std::to_string(port));
        close(fd);
        return -1;
    }

    if (listen(fd, 128) < 0) {
        LOG_ERROR("Reactor", "Failed to listen");
        close(fd);
        return -1;
    }

    LOG_INFO("Reactor", "Listening on port " + std::to_string(port));

    return fd;
}

int Reactor::createEpoll() {
    int fd = epoll_create1(0);
    if (fd < 0) {
        LOG_ERROR("Reactor", "Failed to create epoll");
        return -1;
    }
    return fd;
}

void Reactor::start() {
    running_.store(true);

    // 注册监听socket到epoll
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = listenFd_;
    epoll_ctl(epollFd_, EPOLL_CTL_ADD, listenFd_, &ev);

    // 启动事件循环线程
    for (int i = 0; i < 2; ++i) {
        reactorThreads_.emplace_back([this] { eventLoop(); });
    }
}

void Reactor::stop() {
    running_.store(false);
    for (auto& t : reactorThreads_) {
        if (t.joinable()) t.join();
    }
    threadPool_->stop();
}

void Reactor::removeFromEpoll(int fd) {
    epoll_ctl(epollFd_, EPOLL_CTL_DEL, fd, nullptr);
    close(fd);
}

void Reactor::eventLoop() {
    const int MAX_EVENTS = 64;
    struct epoll_event events[MAX_EVENTS];

    while (running_.load()) {
        int n = epoll_wait(epollFd_, events, MAX_EVENTS, 1000);

        for (int i = 0; i < n; ++i) {
            int fd = events[i].data.fd;

            if (fd == listenFd_) {
                // 新连接
                handleNewConnection();
            } else if (events[i].events & (EPOLLIN | EPOLLERR | EPOLLHUP)) {
                // 可读事件或错误/挂起
                handleRead(fd);
            }
        }
    }
}

void Reactor::handleNewConnection() {
    struct sockaddr_in clientAddr;
    socklen_t clientLen = sizeof(clientAddr);

    int clientFd = accept(listenFd_, (struct sockaddr*)&clientAddr, &clientLen);
    if (clientFd < 0) {
        return;
    }

    // 设置非阻塞
    int flags = fcntl(clientFd, F_GETFL, 0);
    fcntl(clientFd, F_SETFL, flags | O_NONBLOCK);

    // 注册到epoll，使用oneshot防止重复触发
    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLONESHOT;
    ev.data.fd = clientFd;
    if (epoll_ctl(epollFd_, EPOLL_CTL_ADD, clientFd, &ev) < 0) {
        close(clientFd);
        return;
    }
}

void Reactor::handleRead(int fd) {
    char buf[4096];
    ssize_t n = read(fd, buf, sizeof(buf) - 1);

    if (n <= 0) {
        // 连接关闭或错误，移除并关闭
        removeFromEpoll(fd);
        return;
    }

    buf[n] = '\0';
    std::string sql(buf);

    // 去除换行符
    size_t end = sql.find_last_not_of(" \t\r\n");
    if (end == std::string::npos) {
        sql.clear();
    } else {
        sql = sql.substr(0, end + 1);
    }

    if (sql.empty() || !sqlHandler_) {
        removeFromEpoll(fd);
        return;
    }

    // 提交到线程池处理
    int clientFd = fd;
    threadPool_->submit([this, clientFd, sql]() {
        sqlHandler_(clientFd, sql);
        // 重新启用 oneshot，等待下一个请求
        struct epoll_event ev;
        ev.events = EPOLLIN | EPOLLONESHOT;
        ev.data.fd = clientFd;
        epoll_ctl(epollFd_, EPOLL_CTL_MOD, clientFd, &ev);
    });
}

}  // namespace minisql
