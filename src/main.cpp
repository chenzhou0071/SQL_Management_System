#include <iostream>
#include <string>
#include <QCoreApplication>
#include "../common/Logger.h"
#include "../common/Error.h"

#ifdef _WIN32
#include <windows.h>
#endif

using namespace minisql;

// CLI 模式
void runCLI() {
    std::cout << "MiniSQL CLI Mode" << std::endl;
    std::cout << "Type 'exit' or 'quit' to exit." << std::endl;
    std::cout << std::endl;

    std::string line;
    while (true) {
        std::cout << "minisql> ";
        if (!std::getline(std::cin, line)) {
            break;
        }

        // 去除首尾空白
        while (!line.empty() && (line[0] == ' ' || line[0] == '\t')) {
            line = line.substr(1);
        }
        while (!line.empty() && (line.back() == ' ' || line.back() == '\t')) {
            line.pop_back();
        }

        if (line.empty()) continue;

        if (line == "exit" || line == "quit" || line == "q") {
            std::cout << "Goodbye!" << std::endl;
            break;
        }

        // TODO: 调用 SQL 执行器
        std::cout << "SQL: " << line << std::endl;
        std::cout << "Query OK, 0 rows affected" << std::endl;
        std::cout << std::endl;
    }
}

// GUI 模式（占位，后续实现）
void runGUI() {
    // TODO: 启动 Qt 主窗口
    std::cout << "GUI mode not yet implemented, falling back to CLI." << std::endl;
    runCLI();
}

int main(int argc, char* argv[]) {
    // 初始化日志
    Logger::getInstance().init("logs", LogLevel::DEBUG);
    LOG_INFO("Main", "MiniSQL starting...");

    // 检查命令行参数
    bool cliMode = false;
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--cli" || arg == "-c") {
            cliMode = true;
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: minisql [options]" << std::endl;
            std::cout << "Options:" << std::endl;
            std::cout << "  --cli, -c    Force CLI mode" << std::endl;
            std::cout << "  --gui, -g    Force GUI mode (default)" << std::endl;
            std::cout << "  --help, -h   Show this help" << std::endl;
            return 0;
        } else if (arg == "--gui" || arg == "-g") {
            cliMode = false;
        }
    }

    LOG_INFO("Main", cliMode ? "Running in CLI mode" : "Running in GUI mode");

    if (cliMode) {
        runCLI();
    } else {
        runGUI();
    }

    LOG_INFO("Main", "MiniSQL shutting down...");
    return 0;
}
