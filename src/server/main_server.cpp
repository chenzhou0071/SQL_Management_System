// src/server/main_server.cpp
#include "network/TcpServer.h"
#include "transaction/TransactionManager.h"
#include "../common/Logger.h"
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    std::cout << "MiniSQL Server v1.0" << std::endl;
    std::cout << "==================" << std::endl;

    // 解析参数
    int port = 3306;
    std::string dataDir = "./data";

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--port" && i + 1 < argc) {
            port = std::stoi(argv[++i]);
        } else if (arg == "--data-dir" && i + 1 < argc) {
            dataDir = argv[++i];
        }
    }

    // 初始化事务管理器
    minisql::TransactionManager::getInstance().init(dataDir);

    // 启动服务器
    minisql::TcpServer server(port, 4);
    server.setDataDir(dataDir);

    try {
        server.start();
    } catch (const std::exception& e) {
        std::cerr << "Server error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
