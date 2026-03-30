#ifndef TCPCLIENT_H
#define TCPCLIENT_H

#include <QObject>
#include <QTcpSocket>
#include <QString>

class TcpClient : public QObject {
    Q_OBJECT

public:
    explicit TcpClient(QObject *parent = nullptr);
    ~TcpClient();

    bool connectToHost(const QString &host, quint16 port);
    void disconnect();
    bool isConnected() const;
    QString getHost() const { return host_; }
    quint16 getPort() const { return port_; }

    void sendQuery(const QString &sql);

signals:
    void connected();
    void disconnected();
    void responseReady(const QString &data);
    void errorOccurred(const QString &error);

private slots:
    void onReadyRead();
    void onConnected();
    void onDisconnected();

private:
    QTcpSocket *socket_;
    QString host_;
    quint16 port_;
    QByteArray buffer_;
};

#endif // TCPCLIENT_H
