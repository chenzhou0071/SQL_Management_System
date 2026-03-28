// src/server/network/TcpConnection.cpp
#include "TcpConnection.h"
#include <unistd.h>
#include <sys/socket.h>
#include <errno.h>
#include <cstring>

namespace minisql {

TcpConnection::TcpConnection(int fd) : fd_(fd) {}

TcpConnection::~TcpConnection() {
    close();
}

std::string TcpConnection::read() {
    char buf[4096];
    ssize_t n = ::read(fd_, buf, sizeof(buf) - 1);

    if (n > 0) {
        buf[n] = '\0';
        buffer_ += buf;
    } else if (n == 0) {
        // 连接关闭
        return "";
    } else {
        // 错误
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            return "";
        }
    }

    return buffer_;
}

std::string TcpConnection::extractCommand() {
    // 查找换行符
    auto pos = buffer_.find('\n');
    if (pos == std::string::npos) {
        return "";
    }

    std::string command = buffer_.substr(0, pos);
    buffer_.erase(0, pos + 1);

    return command;
}

bool TcpConnection::send(const std::string& data) {
    ssize_t n = ::send(fd_, data.c_str(), data.size(), 0);
    return n == static_cast<ssize_t>(data.size());
}

void TcpConnection::close() {
    if (fd_ >= 0) {
        ::close(fd_);
        fd_ = -1;
    }
}

}  // namespace minisql
