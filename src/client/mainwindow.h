#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QSpinBox>
#include "tcpclient.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onConnectClicked();
    void onSendClicked();
    void onResponseReceived(const QString &data);
    void onConnected();
    void onDisconnected();
    void onError(const QString &error);

private:
    void appendOutput(const QString &text, const QString &color = "white");
    void appendResult(const QStringList &headers, const QList<QStringList> &rows);
    void clearOutput();

    // UI组件
    QLineEdit *hostEdit_;
    QSpinBox *portSpinBox_;
    QPushButton *connectBtn_;
    QTextEdit *outputEdit_;
    QLineEdit *sqlEdit_;
    QPushButton *sendBtn_;
    QLabel *statusLabel_;

    // 网络
    TcpClient *client_;

    // 解析状态
    bool expectColumns_;
    QStringList currentHeaders_;
    QList<QStringList> currentRows_;
    int expectedRows_;
    int receivedRows_;
    bool isError_;
};

#endif // MAINWINDOW_H
