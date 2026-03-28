// src/server/network/TcpServer.cpp
#include "TcpServer.h"
#include "Reactor.h"
#include "SqlProtocol.h"
#include "../../transaction/TransactionManager.h"
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
            auto parseResult = parser::Parser::parseString(sql);
            if (!parseResult.isSuccess()) {
                response.success = false;
                response.message = "Parse error: " + parseResult.getError();
                std::string resp = SqlProtocol::buildResponse(response);
                ::send(fd, resp.c_str(), resp.size(), 0);
                return;
            }

            auto stmt = parseResult.getValue();

            // 获取或创建事务
            auto* txnMgr = &transaction::TransactionManager::getInstance();
            transaction::Transaction* txn = txnMgr->getCurrentTransaction();
            if (!txn) {
                txn = txnMgr->begin();
            }

            // 执行 SQL
            executor::Executor executor(dataDir_);
            auto execResult = executor.execute(txn, stmt);

            if (!execResult.isSuccess()) {
                response.success = false;
                response.message = "Execution error: " + execResult.getError();
                txnMgr->rollback();
            } else {
                response.success = true;
                response.message = "Query OK";
                response.rowCount = execResult.getValue();

                // 获取结果集
                auto& resultSets = executor.getResultSets();
                if (!resultSets.empty() && resultSets[0]) {
                    auto& resultSet = resultSets[0];
                    response.columns = resultSet->columns;
                    response.rows = resultSet->rows;
                }

                txnMgr->commit();
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
