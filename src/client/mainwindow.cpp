#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>
#include <QPlainTextEdit>
#include <QHeaderView>
#include <QScrollBar>
#include <QFont>
#include <QMessageBox>
#include <QFontDatabase>
#include <algorithm>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , client_(new TcpClient(this))
    , expectColumns_(false)
    , expectedRows_(-1)
    , receivedRows_(0)
    , isError_(false)
{
    setWindowTitle("MiniSQL Client");
    resize(900, 600);

    // 创建中央部件
    QWidget *central = new QWidget(this);
    setCentralWidget(central);

    QVBoxLayout *mainLayout = new QVBoxLayout(central);

    // 连接区域
    QHBoxLayout *connLayout = new QHBoxLayout();
    connLayout->addWidget(new QLabel("IP:"));
    hostEdit_ = new QLineEdit();
    hostEdit_->setPlaceholderText("127.0.0.1");
    hostEdit_->setFixedWidth(200);
    connLayout->addWidget(hostEdit_);

    connLayout->addWidget(new QLabel("Port:"));
    portSpinBox_ = new QSpinBox();
    portSpinBox_->setRange(1, 65535);
    portSpinBox_->setValue(3306);
    portSpinBox_->setFixedWidth(80);
    connLayout->addWidget(portSpinBox_);

    connectBtn_ = new QPushButton("Connect");
    connectBtn_->setFixedWidth(80);
    connLayout->addWidget(connectBtn_);

    connLayout->addStretch();

    mainLayout->addLayout(connLayout);

    // 输出区域
    outputEdit_ = new QTextEdit();
    outputEdit_->setReadOnly(true);
    outputEdit_->setFont(QFont("Consolas", 10));
    outputEdit_->setStyleSheet("background-color: #1e1e1e; color: #d4d4d4;");
    mainLayout->addWidget(outputEdit_);

    // SQL输入区域
    QHBoxLayout *sqlLayout = new QHBoxLayout();
    sqlLayout->addWidget(new QLabel("SQL>"));

    sqlEdit_ = new QLineEdit();
    sqlEdit_->setFont(QFont("Consolas", 10));
    sqlEdit_->setStyleSheet("background-color: #2d2d2d; color: #d4d4d4; border: 1px solid #3c3c3c; padding: 4px;");
    sqlLayout->addWidget(sqlEdit_);

    sendBtn_ = new QPushButton("Execute");
    sendBtn_->setFixedWidth(80);
    sendBtn_->setEnabled(false);
    sqlLayout->addWidget(sendBtn_);

    mainLayout->addLayout(sqlLayout);

    // 状态栏
    statusLabel_ = new QLabel("Disconnected");
    statusLabel_->setStyleSheet("color: #888; padding: 4px;");
    mainLayout->addWidget(statusLabel_);

    // 信号连接
    connect(connectBtn_, &QPushButton::clicked, this, &MainWindow::onConnectClicked);
    connect(sendBtn_, &QPushButton::clicked, this, &MainWindow::onSendClicked);
    connect(sqlEdit_, &QLineEdit::returnPressed, this, &MainWindow::onSendClicked);
    connect(client_, &TcpClient::connected, this, &MainWindow::onConnected);
    connect(client_, &TcpClient::disconnected, this, &MainWindow::onDisconnected);
    connect(client_, &TcpClient::responseReady, this, &MainWindow::onResponseReceived);
    connect(client_, &TcpClient::errorOccurred, this, &MainWindow::onError);

    // 默认值
    hostEdit_->setText("127.0.0.1");

    // 初始提示
    appendOutput("MiniSQL Client v1.0", "#888888");
    appendOutput("Type SQL commands and press Enter to execute.", "#888888");
    appendOutput("-----------------------------------", "#555555");
}

MainWindow::~MainWindow() {
}

void MainWindow::onConnectClicked() {
    if (client_->isConnected()) {
        client_->disconnect();
    } else {
        QString host = hostEdit_->text().trimmed();
        int port = portSpinBox_->value();

        if (host.isEmpty()) {
            QMessageBox::warning(this, "Error", "Please enter server IP address");
            return;
        }

        statusLabel_->setText("Connecting...");
        statusLabel_->setStyleSheet("color: #dcdcaa; padding: 4px;");
        connectBtn_->setEnabled(false);
        hostEdit_->setEnabled(false);
        portSpinBox_->setEnabled(false);

        if (!client_->connectToHost(host, port)) {
            // 连接失败 - waitForConnected已经在TcpClient里emit了error
            // 这里按钮会在onError里恢复
        }
    }
}

void MainWindow::onSendClicked() {
    QString sql = sqlEdit_->text().trimmed();
    if (sql.isEmpty()) return;

    // 显示命令
    appendOutput("SQL> " + sql, "#569cd6");
    sqlEdit_->clear();

    // 重置解析状态
    expectColumns_ = false;
    currentHeaders_.clear();
    currentRows_.clear();
    expectedRows_ = -1;
    receivedRows_ = 0;
    isError_ = false;

    // 发送SQL
    client_->sendQuery(sql);
}

void MainWindow::onResponseReceived(const QString &data) {
    if (data == "OK" || data == "ERROR") {
        // 第一行: OK 或 ERROR
        isError_ = (data == "ERROR");
        expectColumns_ = false;
        expectedRows_ = -2;  // 等待解析
        return;
    }

    if (isError_) {
        // 错误消息（data就是完整的错误信息）
        appendOutput("ERROR: " + data, "#f44747");
        appendOutput("", "white");
        return;
    }

    // 解析状态机
    if (expectedRows_ == -2) {
        // 第二行: 可能是 "Query OK" 或 行数
        bool ok;
        int rows = data.toInt(&ok);
        if (ok) {
            // 是数字 = 行数
            expectedRows_ = rows;
            if (rows > 0) {
                expectColumns_ = true;
            }
        } else {
            // 不是数字 = "Query OK" 等消息，继续等待行数
            appendOutput(data, "#4ec9b0");
            expectedRows_ = -1;
        }
        return;
    }

    if (expectedRows_ == -1) {
        // 第三行: 行数
        expectedRows_ = data.toInt();
        if (expectedRows_ > 0) {
            expectColumns_ = true;
        }
        return;
    }

    if (expectColumns_) {
        // 列头行
        currentHeaders_ = data.split(",");
        expectColumns_ = false;
        return;
    }

    if (receivedRows_ < expectedRows_) {
        // 数据行
        currentRows_.append(data.split(","));
        receivedRows_++;

        if (receivedRows_ == expectedRows_) {
            // 显示结果
            if (!currentHeaders_.isEmpty()) {
                appendResult(currentHeaders_, currentRows_);
            }
            appendOutput("", "white");
        }
    }
}

void MainWindow::onConnected() {
    connectBtn_->setText("Disconnect");
    connectBtn_->setEnabled(true);
    sendBtn_->setEnabled(true);
    statusLabel_->setText("Connected: " + client_->getHost() + ":" + QString::number(client_->getPort()));
    statusLabel_->setStyleSheet("color: #4ec9b0; padding: 4px;");

    // 连接成功弹窗
    QMessageBox::information(this, "Connected", "Successfully connected to " + client_->getHost() + ":" + QString::number(client_->getPort()));

    appendOutput("Connected to " + client_->getHost() + ":" + QString::number(client_->getPort()), "#4ec9b0");
    appendOutput("-----------------------------------", "#555555");
}

void MainWindow::onDisconnected() {
    connectBtn_->setText("Connect");
    connectBtn_->setEnabled(true);
    hostEdit_->setEnabled(true);
    portSpinBox_->setEnabled(true);
    sendBtn_->setEnabled(false);
    statusLabel_->setText("Disconnected");
    statusLabel_->setStyleSheet("color: #888; padding: 4px;");

    // 断开连接弹窗
    QMessageBox::information(this, "Disconnected", "Connection to server has been closed.");

    appendOutput("Disconnected", "#dcdcaa");
    appendOutput("-----------------------------------", "#555555");
}

void MainWindow::onError(const QString &error) {
    statusLabel_->setText("Connection failed");
    statusLabel_->setStyleSheet("color: #f44747; padding: 4px;");
    appendOutput("Connection failed: " + error, "#f44747");
    connectBtn_->setEnabled(true);
    hostEdit_->setEnabled(true);
    portSpinBox_->setEnabled(true);

    // 连接失败弹窗
    QMessageBox::critical(this, "Connection Error", "Failed to connect to server:\n" + error);
}

void MainWindow::appendOutput(const QString &text, const QString &color) {
    QTextCharFormat format;
    format.setForeground(QColor(color));
    outputEdit_->setCurrentCharFormat(format);
    outputEdit_->append(text);

    // 滚动到底部
    QTextCursor cursor = outputEdit_->textCursor();
    cursor.movePosition(QTextCursor::End);
    outputEdit_->setTextCursor(cursor);
}

void MainWindow::appendResult(const QStringList &headers, const QList<QStringList> &rows) {
    if (headers.isEmpty()) return;

    // 计算每列宽度
    QList<int> colWidths;
    for (int i = 0; i < headers.size(); ++i) {
        colWidths.append(headers[i].length());
    }

    for (const QStringList &row : rows) {
        for (int i = 0; i < row.size() && i < colWidths.size(); ++i) {
            colWidths[i] = std::max(colWidths[i], static_cast<int>(row[i].length()));
        }
    }

    // 构建分隔线
    QString separator = "+";
    for (int w : colWidths) {
        separator += QString("-").repeated(w + 2) + "+";
    }

    // 显示表头
    appendOutput(separator, "#808080");
    QString headerLine = "|";
    for (int i = 0; i < headers.size(); ++i) {
        headerLine += " " + headers[i].leftJustified(colWidths[i]) + " |";
    }
    appendOutput(headerLine, "#c586c0");
    appendOutput(separator, "#808080");

    // 显示数据行
    for (const QStringList &row : rows) {
        QString rowLine = "|";
        for (int i = 0; i < row.size() && i < colWidths.size(); ++i) {
            rowLine += " " + row[i].leftJustified(colWidths[i]) + " |";
        }
        appendOutput(rowLine, "#9cdcfe");
    }

    appendOutput(separator, "#808080");
    appendOutput(QString::number(rows.size()) + " rows in set", "#608b4e");
}
