// src/server/network/SqlProtocol.h
#pragma once

#include <string>
#include <vector>

namespace minisql {

struct SqlRequest {
    std::string sql;
    uint64_t txnId;  // 0表示无事务
};

struct SqlResponse {
    bool success;
    std::string message;
    int rowCount;
    std::vector<std::string> columns;
    std::vector<std::vector<std::string>> rows;
};

class SqlProtocol {
public:
    // 解析请求
    static SqlRequest parse(const std::string& data);

    // 构建响应
    static std::string buildResponse(const SqlResponse& response);
};

}  // namespace minisql
