#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
class QLineEdit;
class QPushButton;
class QComboBox;
class QTextEdit;
class QTableWidget;
class QTcpSocket;
class QSplitter;
class QAbstractSocket;
class QLabel;
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(MainWindow)

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private:
    void initUI();
    void initConnect();

    //发送Modbus-TCP读保持寄存器请求
    void sendReadRequest(int startAddr, int quantity);
    //发送Modbus-TCP写保持寄存器请求
    void sendWriteRequest(int registerAddr, int writeValue);
    //解析响应报文
    void parseResponse(const QByteArray &data);
    //向日志窗口追加一行
    void appendLog(const QString &text, const QString &color);

private slots:
    void onConnectClicked();
    void onDisconnectClicked();
    void onSendClicked();

    void onSocketReadyRead();
    void onSocketDisconnected();
    void onSocketError();//弃用

private:
    //连接配置
    QLineEdit   *m_ipEdit        = nullptr;
    QLineEdit   *m_portEdit      = nullptr;
    QPushButton *m_connectBtn    = nullptr;
    QPushButton *m_disconnectBtn = nullptr;

    //报文构造
    QComboBox   *m_funcCodeCombo = nullptr;//功能码下拉框
    QLineEdit   *m_startAddrEdit = nullptr;//起始寄存器地址输入框
    QLineEdit   *m_quantityEdit  = nullptr;//寄存器数量输入框
    QPushButton *m_sendBtn       = nullptr;//发送报文按钮

    //原始报文
    QLineEdit   *m_txHexEdit    = nullptr;//发送原始十六进制报文
    QLineEdit   *m_rxHexEdit    = nullptr;//接收原始十六进制报文

    //日志
    QTextEdit   *m_logTextEdit  = nullptr;

    //寄存器表格
    QTableWidget *m_registerTable = nullptr;

    //TCP套接字对象
    QTcpSocket  *m_socket   = nullptr;
    //事务ID，每次请求自增
    quint16     m_transactionId = 0;
    //"数量"那一行的标签
    QLabel *m_quantityOrValueLabel = nullptr;
};

#endif  // MAINWINDOW_H