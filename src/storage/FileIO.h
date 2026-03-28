#pragma once

#include <string>
#include <vector>

#ifdef __linux__
#include "../server/concurrency/LockManager.h"
#endif

namespace minisql {
namespace storage {

class FileIO {
public:
    // 目录操作
    static bool exists(const std::string& path);
    static bool isDirectory(const std::string& path);
    static bool createDirectory(const std::string& path);
    static bool removeDirectory(const std::string& path);
    static std::vector<std::string> listDirectories(const std::string& path);
    static std::vector<std::string> listFiles(const std::string& path, const std::string& extension = "");

    // 文件操作
    static bool existsFile(const std::string& path);
    static bool removeFile(const std::string& path);
    static bool renameFile(const std::string& oldPath, const std::string& newPath);
    static bool writeToFile(const std::string& path, const std::string& content);
    static std::string readFromFile(const std::string& path);
    static bool appendToFile(const std::string& path, const std::string& content);

    // 路径工具
    static std::string join(const std::string& base, const std::string& relative);
    static std::string getDataDir();
    static std::string getDatabaseDir(const std::string& dbName);
};

}  // namespace storage
}  // namespace minisql
