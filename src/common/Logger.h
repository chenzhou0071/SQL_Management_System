#pragma once

#include <string>
#include <memory>
#include <mutex>
#include <fstream>

namespace minisql {

enum class LogLevel {
    DEBUG = 0,
    INFO = 1,
    WARN = 2,
    LVL_ERROR = 3,
    LVL_FATAL = 4
};

class Logger {
public:
    static Logger& getInstance();

    void init(const std::string& logDir = "logs", LogLevel level = LogLevel::INFO);
    void setLevel(LogLevel level);

    void debug(const std::string& module, const std::string& msg);
    void info(const std::string& module, const std::string& msg);
    void warn(const std::string& module, const std::string& msg);
    void error(const std::string& module, const std::string& msg);
    void fatal(const std::string& module, const std::string& msg);

    // 带格式化参数的日志
    void log(LogLevel level, const std::string& module, const std::string& msg);

    // 设置日志文件路径（用于测试）
    void setLogFile(const std::string& path);

private:
    Logger() = default;
    ~Logger();

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    void ensureFileOpened();
    std::string levelToString(LogLevel level);
    std::string currentTime();

    std::ofstream file_;
    LogLevel level_ = LogLevel::INFO;
    std::mutex mutex_;
    std::string logFilePath_ = "logs/minisql.log";
    bool initialized_ = false;
};

// 便捷宏
#define LOG_DEBUG(module, msg) minisql::Logger::getInstance().debug(module, msg)
#define LOG_INFO(module, msg)  minisql::Logger::getInstance().info(module, msg)
#define LOG_WARN(module, msg)  minisql::Logger::getInstance().warn(module, msg)
#define LOG_ERROR(module, msg) minisql::Logger::getInstance().error(module, msg)
#define LOG_FATAL(module, msg) minisql::Logger::getInstance().fatal(module, msg)

}  // namespace minisql
