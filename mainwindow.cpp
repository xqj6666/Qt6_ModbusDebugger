//加入防粘包（）
#include "mainwindow.h"

#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QTextEdit>
#include <QTableWidget>
#include <QSplitter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QTcpSocket>
#include <QDateTime>
#include <QMessageBox>
#include <QLabel>

MainWindow::MainWindow(QWidget *parent)
    :QMainWindow(parent)
{
    m_socket = new QTcpSocket(this);

    initUI();
    initConnect();
}

MainWindow::~MainWindow()
{

}

void MainWindow::initUI()
{
    setWindowTitle("Tcp/Modbus 调试助手");
    resize(1000, 700);

    auto *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    auto *mainLayout = new QHBoxLayout(centralWidget);

    //===左面板===
    auto *leftPanel = new QWidget(this);
    auto *leftLayout = new QVBoxLayout(leftPanel);

    //---连接配置---
    auto *connGroup = new QGroupBox("连接配置", leftPanel);
    //表单:标签列+输入控件类，不需要手动维护行号和列号
    auto *connLayout = new QFormLayout(connGroup);

    m_ipEdit = new QLineEdit("127.0.0.1", connGroup);
    m_portEdit = new QLineEdit("502", connGroup);

    auto *btnLayout = new QHBoxLayout();
    m_connectBtn = new QPushButton("连接", connGroup);
    m_disconnectBtn = new QPushButton("断开", connGroup);
    m_disconnectBtn->setEnabled(false);
    btnLayout->addWidget(m_connectBtn);
    btnLayout->addWidget(m_disconnectBtn);

    connLayout->addRow("IP 地址：", m_ipEdit);
    connLayout->addRow("端口号:", m_portEdit);
    connLayout->addRow("", btnLayout);
    leftLayout->addWidget(connGroup);

    //---报文构造---
    auto *msgGroup = new QGroupBox("报文构造", leftPanel);
    auto *msgLayout = new QFormLayout(msgGroup);

    m_funcCodeCombo = new QComboBox(msgGroup);
    m_funcCodeCombo->addItem("03 - 读保持寄存器", 0x03);
    m_funcCodeCombo->addItem("06 - 写单个寄存器", 0x06);

    m_startAddrEdit = new QLineEdit("0", msgGroup);
    m_quantityEdit  = new QLineEdit("10", msgGroup);
    m_quantityOrValueLabel = new QLabel("数量：", msgGroup);//随着功能码变化
    m_sendBtn = new QPushButton("发送报文", msgGroup);

    msgLayout->addRow("功能码:", m_funcCodeCombo);
    msgLayout->addRow("起始地址:", m_startAddrEdit);
    msgLayout->addRow(m_quantityOrValueLabel, m_quantityEdit);
    msgLayout->addRow("", m_sendBtn);
    leftLayout->addWidget(msgGroup);

    //---原始报文---
    auto *hexGroup = new QGroupBox("原始报文", leftPanel);
    auto *hexLayout = new QFormLayout(hexGroup);

    m_txHexEdit = new QLineEdit(hexGroup);
    m_txHexEdit->setReadOnly(true);
    m_txHexEdit->setPlaceholderText("TX: 发送的报文...");
    m_rxHexEdit = new QLineEdit(hexGroup);
    m_rxHexEdit->setReadOnly(true);
    m_rxHexEdit->setPlaceholderText("RX: 接收报文...");

    hexLayout->addRow("TX:",m_txHexEdit);
    hexLayout->addRow("RX:",m_rxHexEdit);
    leftLayout->addWidget(hexGroup);

    leftLayout->addStretch();

    //===右面板:收发日志===
    m_logTextEdit = new QTextEdit(this);//为什么不用QLabel
    m_logTextEdit->setReadOnly(true);
    m_logTextEdit->setStyleSheet(
        "QTextEdit {"
        "    background-color: #1e1e1e;"
        "    color: #dcdcdc;"
        "    font-family: Consolas, monospace;"
        "    font-size: 12px;"
        "}"
        );

    //左右分割
    auto *topSplitter = new QSplitter(Qt::Horizontal, this);
    topSplitter->addWidget(leftPanel);
    topSplitter->addWidget(m_logTextEdit);
    topSplitter->setStretchFactor(0,1);//什么意思？？？
    topSplitter->setStretchFactor(1,2);

    //===底部：寄存器表格===
    m_registerTable = new QTableWidget(this);
    m_registerTable->setColumnCount(4);
    m_registerTable->setHorizontalHeaderLabels({"地址","名称","十六进制值","十进制值"});
    m_registerTable->horizontalHeader()->setStretchLastSection(true);
    m_registerTable->verticalHeader()->setVisible(false);
    m_registerTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_registerTable->setSelectionBehavior(QAbstractItemView::SelectRows);

    // 上下分割
    auto *mainSplitter = new QSplitter(Qt::Vertical, this);
    mainSplitter->addWidget(topSplitter);
    mainSplitter->addWidget(m_registerTable);
    mainSplitter->setStretchFactor(0, 3);
    mainSplitter->setStretchFactor(1, 1);

    mainLayout->addWidget(mainSplitter);
}

void MainWindow::initConnect()
{
    //---按钮--
    connect(m_connectBtn, &QPushButton::clicked, this, &MainWindow::onConnectClicked);
    connect(m_disconnectBtn, &QPushButton::clicked, this, &MainWindow::onDisconnectClicked);
    connect(m_sendBtn, &QPushButton::clicked, this, &MainWindow::onSendClicked);

    //---Socket信号---
    connect(m_socket, &QTcpSocket::readyRead, this, &MainWindow::onSocketReadyRead);
    connect(m_socket, &QTcpSocket::disconnected, this, &MainWindow::onSocketDisconnected);
    connect(m_socket, &QTcpSocket::errorOccurred, this, [this](QAbstractSocket::SocketError error) {
        Q_UNUSED(error);
        if (m_socket->state() != QAbstractSocket::ConnectedState) {
            appendLog("连接错误: " + m_socket->errorString(), "red");
        }
    });

    connect(m_funcCodeCombo, &QComboBox::currentIndexChanged, this, [this](int index) {
        Q_UNUSED(index);
        int funcCode = m_funcCodeCombo->currentData().toInt();
        if (funcCode == 0x03) {
            m_quantityOrValueLabel->setText("数量:");
            m_quantityEdit->setPlaceholderText("读取数量");
        } else if (funcCode == 0x06) {
            m_quantityOrValueLabel->setText("写入值:");
            m_quantityEdit->setPlaceholderText("写入值");
        }
    });
}

//===日志工具函数===
void MainWindow::appendLog(const QString &text, const QString &color)
{
    QString timestamp = QDateTime::currentDateTime().toString("HH:mm:ss");
    m_logTextEdit->setTextColor(QColor(color));
    m_logTextEdit->append(QString("[%1] %2").arg(timestamp, text));
}

//===TCP连接===
void MainWindow::onConnectClicked()
{
    QString ip = m_ipEdit->text().trimmed();
    quint16 port = m_portEdit->text().toUShort();

    if(ip.isEmpty() || port == 0){
        QMessageBox::warning(this, "参数错误", "IP地址和端口号不能为空");
        return;
    }
    m_socket->connectToHost(ip,port);

    appendLog(QString("正在连接%1:%2...").arg(ip).arg(port),"yellow");

    if(m_socket->waitForConnected(3000)){
        appendLog("连接成功", "green");
        m_connectBtn->setEnabled(false);
        m_disconnectBtn->setEnabled(true);
    }
    else{
        appendLog("连接失败：" + m_socket->errorString(), "red");
    }
}

void MainWindow::onDisconnectClicked()
{
    m_socket->disconnectFromHost();
}

void MainWindow::onSocketDisconnected()
{
    appendLog("连接已断开", "yellow");
    m_connectBtn->setEnabled(true);
    m_disconnectBtn->setEnabled(false);
}

void MainWindow::onSocketError()//QAbstractSocket在头文件的枚举类型无法inclue识别，用lambda代替
{

}

//===发送Modbus-TCP读请求===
void MainWindow::onSendClicked()
{
    if(m_socket->state() != QAbstractSocket::ConnectedState){
        QMessageBox::warning(this, "未连接", "请先连接设备");
        return;
    }

    int funcCode = m_funcCodeCombo->currentData().toInt();
    int startAddr = m_startAddrEdit->text().toInt();
    int valueOrQuantity = m_quantityEdit->text().toInt();

    if(funcCode == 0x03)
    {
        sendReadRequest(startAddr, valueOrQuantity);
    }
    else if(funcCode == 0x06)
    {
        sendWriteRequest(startAddr, valueOrQuantity);
    }

}

void MainWindow::sendReadRequest(int startAddr, int quantity)
{
    //---手动组装Modbus-TCP报文---
    QByteArray txData;
    txData.resize(12);

    //事务ID(2字节，大端序)
    txData[0] = static_cast<char>((m_transactionId >> 8) & 0xFF);
    txData[1] = static_cast<char>(m_transactionId & 0xFF);

    //协议ID(2字节，固定0)
    txData[2] = 0x00;
    txData[3] = 0x00;

    //后续长度(2字节,固定6)
    txData[4] = 0x00;
    txData[5] = 0x06;

    //单元ID(1字节)
    txData[6] = 0x01;

    //功能码(1字节)
    txData[7] = static_cast<char>(0x03);

    //起始地址(2字节，大端序)
    txData[8] = static_cast<char>((startAddr >> 8) & 0xFF);
    txData[9] = static_cast<char>(startAddr & 0xFF);

    //寄存器数量(2字节，大端序)
    txData[10] = static_cast<char>((quantity >> 8) & 0xFF);
    txData[11] = static_cast<char>(quantity & 0xFF);

    //---发送---
    m_socket->write(txData);
    m_transactionId++;

    //---显示原始报文---
    QString hexStr = txData.toHex(' ').toUpper();
    m_txHexEdit->setText(hexStr);
    appendLog("Tx: " + hexStr, "#4fc3f7");//蓝色
}

void MainWindow::sendWriteRequest(int registerAddr, int writeValue)
{
    //手动组装Modbus-TCP 写单个寄存器报文
    QByteArray txData;
    txData.resize(12);

    //事务ID
    txData[0] = static_cast<char>((m_transactionId >> 8) & 0xFF);
    txData[1] = static_cast<char>(m_transactionId & 0xFF);

    //协议ID
    txData[2] = 0x00;
    txData[3] = 0x00;

    //后续长度（固定6）
    txData[4] = 0x00;
    txData[5] = 0x06;

    //单元ID
    txData[6] = 0x01;

    //功能码0x06
    txData[7] = 0x06;

    //起始地址
    txData[8] = static_cast<char>((registerAddr >> 8) & 0xFF);
    txData[9] = static_cast<char>(registerAddr & 0xFF);

    //写入值(写入报文最好2字节是值，不是数量)
    txData[10] = static_cast<char>((writeValue >> 8) & 0xFF);
    txData[11] = static_cast<char>(writeValue & 0xFF);

    //发送
    m_socket->write(txData);
    m_transactionId++;

    //显示原始报文
    QString hexStr = txData.toHex(' ').toUpper();
    m_txHexEdit->setText(hexStr);
    appendLog("TX：" + hexStr, "#4fc3f7");
}

//===接收响应===
void MainWindow::onSocketReadyRead()
{
    QByteArray rxData = m_socket->readAll();

    //显示原始报文
    QString hexStr = rxData.toHex(' ').toUpper();
    m_rxHexEdit->setText(hexStr);
    appendLog("RX：" + hexStr, "#81c784");//绿色

    //解析响应
    parseResponse(rxData);
}

void MainWindow::parseResponse(const QByteArray &data)
{
    //最小响应长度：MBAP头(7) + 功能码(1) + 数据长度(1) = 9
    if(data.size() < 9){
        appendLog("响应数据太短，无法解析", "red");
        return;
    }

    //解析MBAP头
    quint16 transId = (static_cast<quint8>(data[0]) << 8) | static_cast<quint8>(data[1]);
    quint8 funcCode = static_cast<quint8>(data[7]);

    appendLog(QString("事务ID：%1， 功能码：0x%2")
                  .arg(transId)
                  .arg(funcCode,2,16,QChar('0'))
                  .toUpper(),"white");

    //检查是否异常响应
    if(funcCode & 0x80){
        quint8 exceptionCode = static_cast<quint8>(data[8]);
        appendLog(QString("异常响应！异常码: 0x%1").arg(exceptionCode, 2, 16, QChar('0')), "red");
        return;
    }

    //解析功能码0x03的正常响应
    if(funcCode == 0x03)
    {
        quint8 byteCount = static_cast<quint8>(data[8]);
        int registerCount = byteCount / 2;

        //获取起始地址(从发送的报文里取，或者从响应推断)
        int startAddr = m_startAddrEdit->text().toInt();

        //更新表格
        m_registerTable->setRowCount(registerCount);
        for(int i=0; i < registerCount; ++i)
        {
            int offset = 9 + i * 2;
            if(offset + 1 >= data.size()){break;}

            quint16 value = (static_cast<quint8>(data[offset]) << 8) | static_cast<quint8>(data[offset + 1]);
            m_registerTable->setItem(i, 0, new QTableWidgetItem(QString("0x%1").arg(startAddr + i, 4, 16, QChar('0'))));
            m_registerTable->setItem(i, 1, new QTableWidgetItem(QString("寄存器_%1").arg(startAddr + i)));
            m_registerTable->setItem(i, 2, new QTableWidgetItem(QString("0x%1").arg(value, 4, 16, QChar('0'))));
            m_registerTable->setItem(i, 3, new QTableWidgetItem(QString::number(value)));
        }
        appendLog(QString("读取成功：%1 个寄存器").arg(registerCount), "green");
    }
}















