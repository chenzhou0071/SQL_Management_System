#include "tcpclient.h"
#include <QDebug>

TcpClient::TcpClient(QObject *parent)
    : QObject(parent)
    , socket_(new QTcpSocket(this))
    , port_(0)
{
    connect(socket_, &QTcpSocket::connected, this, &TcpClient::onConnected);
    connect(socket_, &QTcpSocket::disconnected, this, &TcpClient::onDisconnected);
    connect(socket_, &QTcpSocket::readyRead, this, &TcpClient::onReadyRead);
    connect(socket_, &QTcpSocket::errorOccurred, [this](QAbstractSocket::SocketError) {
        emit errorOccurred(socket_->errorString());
    });
}

TcpClient::~TcpClient() {
    if (socket_->isOpen()) {
        socket_->disconnectFromHost();
    }
}

bool TcpClient::connectToHost(const QString &host, quint16 port) {
    host_ = host;
    port_ = port;

    socket_->connectToHost(host, port);
    if (!socket_->waitForConnected(5000)) {
        emit errorOccurred(socket_->errorString());
        return false;
    }
    return true;
}

void TcpClient::disconnect() {
    socket_->disconnectFromHost();
}

bool TcpClient::isConnected() const {
    return socket_->state() == QAbstractSocket::ConnectedState;
}

void TcpClient::sendQuery(const QString &sql) {
    if (!isConnected()) {
        emit errorOccurred("Not connected");
        return;
    }

    QByteArray data = sql.toUtf8();
    data.append('\n');
    socket_->write(data);
    socket_->flush();
}

void TcpClient::onReadyRead() {
    buffer_.append(socket_->readAll());

    // 按行处理响应
    while (buffer_.contains('\n')) {
        int index = buffer_.indexOf('\n');
        QByteArray line = buffer_.left(index);
        buffer_ = buffer_.mid(index + 1);

        // 跳过空行
        if (!line.isEmpty()) {
            emit responseReady(QString::fromUtf8(line));
        }
    }
}

void TcpClient::onConnected() {
    emit connected();
}

void TcpClient::onDisconnected() {
    emit disconnected();
}
