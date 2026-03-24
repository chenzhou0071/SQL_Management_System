#include "Logger.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <ctime>
#include <mutex>
#include <fstream>

using namespace minisql;

Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

Logger::~Logger() {
    if (file_.is_open()) {
        file_.flush();
        file_.close();
    }
}

void Logger::init(const std::string& logDir, LogLevel level) {
    std::lock_guard<std::mutex> lock(mutex_);

    logFilePath_ = logDir + "/minisql.log";
    level_ = level;
    initialized_ = true;

    // 确保日志目录存在
    ensureFileOpened();
}

void Logger::setLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(mutex_);
    level_ = level;
}

void Logger::setLogFile(const std::string& path) {
    std::lock_guard<std::mutex> lock(mutex_);
    logFilePath_ = path;
    if (file_.is_open()) {
        file_.close();
    }
    ensureFileOpened();
}

void Logger::ensureFileOpened() {
    if (!file_.is_open()) {
        file_.open(logFilePath_, std::ios::app);
        if (!file_.is_open()) {
            std::cerr << "[FATAL] Cannot open log file: " << logFilePath_ << std::endl;
        }
    }
}

std::string Logger::currentTime() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;

    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return ss.str();
}

std::string Logger::levelToString(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG:    return "DEBUG";
        case LogLevel::INFO:     return "INFO ";
        case LogLevel::WARN:     return "WARN ";
        case LogLevel::LVL_ERROR: return "ERROR";
        case LogLevel::LVL_FATAL: return "FATAL";
        default:                 return "UNKWN";
    }
}

void Logger::log(LogLevel level, const std::string& module, const std::string& msg) {
    if (level < level_) return;

    std::lock_guard<std::mutex> lock(mutex_);

    std::stringstream ss;
    ss << "[" << currentTime() << "] "
       << "[" << levelToString(level) << "] "
       << "[" << module << "] "
       << msg;

    std::string line = ss.str();

    // 控制台输出
    std::cout << line << std::endl;

    // 写入文件
    if (file_.is_open()) {
        file_ << line << std::endl;
        file_.flush();
    }
}

void Logger::debug(const std::string& module, const std::string& msg) {
    log(LogLevel::DEBUG, module, msg);
}

void Logger::info(const std::string& module, const std::string& msg) {
    log(LogLevel::INFO, module, msg);
}

void Logger::warn(const std::string& module, const std::string& msg) {
    log(LogLevel::WARN, module, msg);
}

void Logger::error(const std::string& module, const std::string& msg) {
    log(LogLevel::LVL_ERROR, module, msg);
}

void Logger::fatal(const std::string& module, const std::string& msg) {
    log(LogLevel::LVL_FATAL, module, msg);
}
