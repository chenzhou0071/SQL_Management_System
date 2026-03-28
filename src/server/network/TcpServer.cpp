// src/server/network/TcpServer.cpp
#include "TcpServer.h"
#include "Reactor.h"
#include "SqlProtocol.h"
#include "../../common/Logger.h"
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>

namespace minisql {

// 全局服务器指针,用于信号处理
static TcpServer* g_server = nullptr;

void signalHandler(int sig) {
    if (g_server) {
        LOG_INFO("TcpServer", "Received signal " + std::to_string(sig) + ", shutting down...");
        g_server->stop();
    }
}

TcpServer::TcpServer(int port, int threadCount)
    : port_(port), threadCount_(threadCount) {
    reactor_ = std::make_unique<Reactor>(port, threadCount);

    // 设置SQL处理函数
    reactor_->setSqlHandler([this](int fd, const std::string& sql) {
        SqlResponse response;

        // TODO: 调用执行器执行SQL
        // 这里先实现简单的响应
        if (sql == "SELECT 1;" || sql == "SELECT 1") {
            response.success = true;
            response.message = "Query OK";
            response.rowCount = 1;
            response.columns = {"1"};
            response.rows = {{"1"}};
        } else if (sql.find("SELECT") == 0) {
            response.success = true;
            response.message = "Query OK";
            response.rowCount = 0;
        } else if (sql == "SHOW TABLES;" || sql == "SHOW DATABASES;") {
            response.success = true;
            response.message = "Query OK";
            response.rowCount = 0;
        } else {
            response.success = true;
            response.message = "OK";
            response.rowCount = 0;
        }

        std::string resp = SqlProtocol::buildResponse(response);
        ::send(fd, resp.c_str(), resp.size(), 0);
    });
}

TcpServer::~TcpServer() {
    stop();
}

void TcpServer::start() {
    // 注册信号处理
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    g_server = this;

    LOG_INFO("TcpServer", "Starting server on port " + std::to_string(port_));

    reactor_->start();

    // 主线程等待
    while (true) {
        sleep(1);
    }
}

void TcpServer::stop() {
    reactor_->stop();
    LOG_INFO("TcpServer", "Server stopped");
}

}  // namespace minisql
