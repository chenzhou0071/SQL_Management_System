// src/server/network/SqlProtocol.cpp
#include "SqlProtocol.h"
#include <sstream>
#include <algorithm>

namespace minisql {

SqlRequest SqlProtocol::parse(const std::string& data) {
    SqlRequest request;
    request.txnId = 0;  // TODO: 解析事务ID

    // 简单解析:去除首尾空白
    std::string sql = data;
    sql.erase(0, sql.find_first_not_of(" \t\r\n"));
    sql.erase(sql.find_last_not_of(" \t\r\n") + 1);

    request.sql = sql;
    return request;
}

std::string SqlProtocol::buildResponse(const SqlResponse& response) {
    std::ostringstream oss;

    if (response.success) {
        oss << "OK\n";
        oss << response.message << "\n";
        oss << response.rowCount << "\n";

        // 列名
        for (size_t i = 0; i < response.columns.size(); ++i) {
            if (i > 0) oss << ",";
            oss << response.columns[i];
        }
        oss << "\n";

        // 数据行
        for (const auto& row : response.rows) {
            for (size_t i = 0; i < row.size(); ++i) {
                if (i > 0) oss << ",";
                oss << row[i];
            }
            oss << "\n";
        }
    } else {
        oss << "ERROR\n";
        oss << response.message << "\n";
    }

    return oss.str();
}

}  // namespace minisql
