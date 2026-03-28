// src/server/network/TcpServer.cpp
#include "TcpServer.h"
#include "Reactor.h"
#include "SqlProtocol.h"
#include "../transaction/TransactionManager.h"
#include "../../executor/Executor.h"
#include "../../parser/Parser.h"
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

        try {
            // 解析 SQL
            parser::Lexer lexer(sql);
            parser::Parser parser(lexer);
            auto stmt = parser.parseStatement();

            // 执行 SQL
            auto& executor = executor::Executor::getInstance();
            auto execResult = executor.execute(stmt.get());

            if (!execResult.isSuccess()) {
                response.success = false;
                response.message = "Execution error: " + execResult.getError();
            } else {
                response.success = true;
                response.message = "Query OK";

                // 获取列名和数据
                auto& result = execResult.getValue();
                response.rowCount = result.getRowCount();
                response.columns = result.columnNames;

                // 转换 Tuple -> vector<string>
                for (auto& row : result.rows) {
                    std::vector<std::string> rowStr;
                    for (auto& val : row) {
                        rowStr.push_back(val.toString());
                    }
                    response.rows.push_back(std::move(rowStr));
                }
            }

        } catch (const std::exception& e) {
            response.success = false;
            response.message = std::string("Error: ") + e.what();
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
