#include "FileIO.h"
#include "../common/Logger.h"
#include <fstream>
#include <filesystem>
#include <iostream>
#include <functional>
#include <memory>

#ifdef __linux__
#include "../server/concurrency/LockManager.h"
#endif

#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#else
#include <sys/stat.h>
#include <unistd.h>
#endif

using namespace minisql;
using namespace storage;

namespace fs = std::filesystem;

bool FileIO::exists(const std::string& path) {
    try {
        return fs::exists(path);
    } catch (...) {
        return false;
    }
}

bool FileIO::isDirectory(const std::string& path) {
    try {
        return fs::is_directory(path);
    } catch (...) {
        return false;
    }
}

bool FileIO::createDirectory(const std::string& path) {
    try {
        return fs::create_directories(path);
    } catch (const std::exception& e) {
        LOG_ERROR("FileIO", std::string("Failed to create directory: ") + e.what());
        return false;
    }
}

bool FileIO::removeDirectory(const std::string& path) {
    try {
        return fs::remove_all(path);
    } catch (const std::exception& e) {
        LOG_ERROR("FileIO", std::string("Failed to remove directory: ") + e.what());
        return false;
    }
}

std::vector<std::string> FileIO::listDirectories(const std::string& path) {
    std::vector<std::string> dirs;
    try {
        for (const auto& entry : fs::directory_iterator(path)) {
            if (fs::is_directory(entry)) {
                dirs.push_back(entry.path().filename().string());
            }
        }
    } catch (const std::exception& e) {
        LOG_WARN("FileIO", std::string("Failed to list directories: ") + e.what());
    }
    return dirs;
}

std::vector<std::string> FileIO::listFiles(const std::string& path, const std::string& extension) {
    std::vector<std::string> files;
    try {
        for (const auto& entry : fs::directory_iterator(path)) {
            if (fs::is_regular_file(entry)) {
                std::string filename = entry.path().filename().string();
                if (extension.empty() || (filename.size() >= extension.size() &&
                    filename.substr(filename.size() - extension.size()) == extension)) {
                    size_t dotPos = filename.rfind('.');
                    if (dotPos != std::string::npos && dotPos > 0) {
                        files.push_back(filename.substr(0, dotPos));
                    } else {
                        files.push_back(filename);
                    }
                }
            }
        }
    } catch (const std::exception& e) {
        LOG_WARN("FileIO", std::string("Failed to list files: ") + e.what());
    }
    return files;
}

bool FileIO::existsFile(const std::string& path) {
    try {
        return fs::exists(path) && fs::is_regular_file(path);
    } catch (...) {
        return false;
    }
}

bool FileIO::removeFile(const std::string& path) {
    try {
        return fs::remove(path);
    } catch (const std::exception& e) {
        LOG_ERROR("FileIO", std::string("Failed to remove file: ") + e.what());
        return false;
    }
}

bool FileIO::renameFile(const std::string& oldPath, const std::string& newPath) {
    try {
        fs::rename(oldPath, newPath);
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("FileIO", std::string("Failed to rename file: ") + e.what());
        return false;
    }
}

bool FileIO::writeToFile(const std::string& path, const std::string& content) {
    try {
#ifdef __linux__
        minisql::LockManager::getInstance().acquireFileLock(path);
        auto finally = std::unique_ptr<bool, std::function<void(bool*)>>(
            new bool(true),
            [&path](bool*) {
                minisql::LockManager::getInstance().releaseFileLock(path);
            }
        );
#endif
        std::ofstream file(path);
        if (!file.is_open()) {
            LOG_ERROR("FileIO", std::string("Cannot open file for writing: ") + path);
            return false;
        }
        file << content;
        file.close();
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("FileIO", std::string("Failed to write file: ") + e.what());
        return false;
    }
}

std::string FileIO::readFromFile(const std::string& path) {
    try {
#ifdef __linux__
        minisql::LockManager::getInstance().acquireFileLock(path);
        auto finally = std::unique_ptr<bool, std::function<void(bool*)>>(
            new bool(true),
            [&path](bool*) {
                minisql::LockManager::getInstance().releaseFileLock(path);
            }
        );
#endif
        std::ifstream file(path);
        if (!file.is_open()) {
            LOG_ERROR("FileIO", std::string("Cannot open file for reading: ") + path);
            return "";
        }
        std::stringstream buffer;
        buffer << file.rdbuf();
        file.close();
        return buffer.str();
    } catch (const std::exception& e) {
        LOG_ERROR("FileIO", std::string("Failed to read file: ") + e.what());
        return "";
    }
}

bool FileIO::appendToFile(const std::string& path, const std::string& content) {
    try {
#ifdef __linux__
        minisql::LockManager::getInstance().acquireFileLock(path);
        auto finally = std::unique_ptr<bool, std::function<void(bool*)>>(
            new bool(true),
            [&path](bool*) {
                minisql::LockManager::getInstance().releaseFileLock(path);
            }
        );
#endif
        std::ofstream file(path, std::ios::app);
        if (!file.is_open()) {
            LOG_ERROR("FileIO", std::string("Cannot open file for appending: ") + path);
            return false;
        }
        file << content;
        file.close();
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("FileIO", std::string("Failed to append file: ") + e.what());
        return false;
    }
}

std::string FileIO::join(const std::string& base, const std::string& relative) {
    try {
        return (fs::path(base) / relative).string();
    } catch (...) {
        return base + "/" + relative;
    }
}

std::string FileIO::getDataDir() {
    return "data";
}

std::string FileIO::getDatabaseDir(const std::string& dbName) {
    return join(getDataDir(), dbName);
}
