// src/server/network/TcpServer.cpp
#include "TcpServer.h"
#include "Reactor.h"
#include "SqlProtocol.h"
#include "../transaction/TransactionManager.h"
#include "../../executor/Executor.h"
#include "../../parser/Parser.h"
#include "../../common/Logger.h"
#include "../../common/Error.h"
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <cstring>
#include <errno.h>

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
        LOG_INFO("TcpServer", "=== SQL Handler Start ===");
        LOG_INFO("TcpServer", "Received SQL: [" + sql + "]");
        LOG_INFO("TcpServer", "SQL length: " + std::to_string(sql.length()));

        // 获取当前连接的会话
        auto& session = getOrCreateSession(fd);
        std::string currentDb = session.getCurrentDatabase();

        SqlResponse response;

        try {
            LOG_INFO("TcpServer", "Step 1: Creating Lexer...");
            // 解析 SQL
            parser::Lexer lexer(sql);
            LOG_INFO("TcpServer", "Step 2: Lexer created, creating Parser...");

            parser::Parser parser(lexer);
            LOG_INFO("TcpServer", "Step 3: Parser created, parsing statement...");

            auto stmt = parser.parseStatement();
            LOG_INFO("TcpServer", "Step 4: Statement parsed, type: " + std::to_string(static_cast<int>(stmt->getType())));

            // 执行 SQL（使用会话的数据库）
            LOG_INFO("TcpServer", "Step 5: Getting Executor instance...");
            auto& executor = executor::Executor::getInstance();
            LOG_INFO("TcpServer", "Step 6: Executor instance obtained, executing statement...");
            LOG_INFO("TcpServer", "Step 6b: Using database from session: [" + currentDb + "]");

            auto execResult = executor.execute(stmt.get(), currentDb);
            LOG_INFO("TcpServer", "Step 7: Statement executed, checking result...");

            // 如果是 USE 语句，更新会话的数据库
            if (execResult.isSuccess() && stmt->getType() == parser::ASTNodeType::USE_STMT) {
                auto useStmt = dynamic_cast<parser::UseStmt*>(stmt.get());
                if (useStmt) {
                    session.setCurrentDatabase(useStmt->database);
                    LOG_INFO("TcpServer", "Session database updated to: " + useStmt->database);
                }
            }

            if (!execResult.isSuccess()) {
                LOG_INFO("TcpServer", "Step 8a: Execution failed");
                response.success = false;
                response.message = execResult.getError().what();
                LOG_INFO("TcpServer", "Error message: " + response.message);
            } else {
                LOG_INFO("TcpServer", "Step 8b: Execution succeeded");
                response.success = true;
                response.message = "Query OK";

                // 获取列名和数据
                LOG_INFO("TcpServer", "Step 9: Getting result...");
                auto* result = execResult.getValue();
                if (result) {
                    LOG_INFO("TcpServer", "Step 10: Result obtained, row count: " + std::to_string(result->getRowCount()));
                    response.rowCount = result->getRowCount();
                    response.columns = result->columnNames;

                    // 转换 Tuple -> vector<string>
                    LOG_INFO("TcpServer", "Step 11: Converting rows...");
                    for (auto& row : result->rows) {
                        std::vector<std::string> rowStr;
                        for (auto& val : row) {
                            rowStr.push_back(val.toString());
                        }
                        response.rows.push_back(std::move(rowStr));
                    }
                    LOG_INFO("TcpServer", "Step 12: Rows converted, count: " + std::to_string(response.rows.size()));
                } else {
                    LOG_INFO("TcpServer", "Step 10: No result data");
                }
            }

        } catch (const MiniSQLException& e) {
            LOG_ERROR("TcpServer", "MiniSQLException caught: " + std::string(e.what()));
            response.success = false;
            response.message = e.what();
        } catch (const std::exception& e) {
            LOG_ERROR("TcpServer", "std::exception caught: " + std::string(e.what()));
            response.success = false;
            response.message = std::string("Internal error: ") + e.what();
        } catch (...) {
            LOG_ERROR("TcpServer", "Unknown exception caught");
            response.success = false;
            response.message = "Unknown error occurred";
        }

        LOG_INFO("TcpServer", "Step 13: Building response...");
        LOG_INFO("TcpServer", "  Columns count: " + std::to_string(response.columns.size()));
        LOG_INFO("TcpServer", "  Rows count: " + std::to_string(response.rows.size()));
        for (size_t i = 0; i < response.columns.size(); ++i) {
            LOG_INFO("TcpServer", "  Column[" + std::to_string(i) + "]: " + response.columns[i]);
        }
        std::string resp = SqlProtocol::buildResponse(response);
        LOG_INFO("TcpServer", "Step 14: Response built, size: " + std::to_string(resp.size()));
        LOG_INFO("TcpServer", "Step 15: Sending response...");

        // 直接发送响应
        ssize_t sent = ::send(fd, resp.c_str(), resp.size(), 0);
        if (sent < 0) {
            LOG_ERROR("TcpServer", "send() failed: " + std::string(strerror(errno)));
        } else {
            LOG_INFO("TcpServer", "Response sent, bytes: " + std::to_string(sent));
        }

        LOG_INFO("TcpServer", "=== SQL Handler End ===");
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

    // 清理所有会话
    {
        std::lock_guard<std::mutex> lock(sessionsMutex_);
        sessions_.clear();
    }

    LOG_INFO("TcpServer", "Server stopped");
}

server::Session& TcpServer::getOrCreateSession(int fd) {
    std::lock_guard<std::mutex> lock(sessionsMutex_);
    return sessions_[fd];  // 自动创建或返回已有Session
}

void TcpServer::removeSession(int fd) {
    std::lock_guard<std::mutex> lock(sessionsMutex_);
    sessions_.erase(fd);
}

}  // namespace minisql
