// src/server/network/Session.h
#pragma once

#include <string>

namespace minisql {
namespace server {

// 会话状态 - 每个TCP连接对应一个Session实例
class Session {
public:
    Session() : currentDatabase_("") {}

    // 获取当前数据库名称
    const std::string& getCurrentDatabase() const {
        return currentDatabase_;
    }

    // 设置当前数据库名称
    void setCurrentDatabase(const std::string& dbName) {
        currentDatabase_ = dbName;
    }

private:
    std::string currentDatabase_;
};

} // namespace server
} // namespace minisql
