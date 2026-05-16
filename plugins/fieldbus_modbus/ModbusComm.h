#ifndef MODBUSCOMM_H
#define MODBUSCOMM_H

#include <QObject>
#include <QTimer>
#include <QQueue>
#include <QMutex>
#include <QAtomicInt>
typedef struct _modbus modbus_t;
/**
 * @brief Modbus 连接配置参数
 *
 * 包含 TCP 和 RTU 两种连接模式所需的所有参数。
 */
struct ModbusConfig{
    enum ConnectionType { Serial, Tcp };
    ConnectionType type = Tcp;          //< 连接类型：TCP网口 / Serial串口
    QString host = "127.0.0.1";         //< TCP模式下的目标主机IP
    int port = 502;                     //< TCP模式下的端口号
    QString portName = "COM1";          //< RTU模式下的串口名
    int baudRate = 9600;                //< RTU波特率
    char parity = 'N';                  //< RTU校验位：N=None, E=Even, O=Odd
    int dataBit = 8;                    //< RTU数据位
    int stopBit = 1;                    //< RTU停止位
    int timeout = 1000;                 //< 响应超时，单位毫秒
    int retries = 3;                    //< 失败重试次数
    int poolInterval = 500;             //< 轮询间隔，单位毫秒
    int heartbeatInterval = 5000;       //< 心跳检测间隔，单位毫秒
};
class ModbusComm : public QObject
{
    Q_OBJECT
public:
    explicit ModbusComm(QObject *parent = nullptr);
    ~ModbusComm();
    // @name 连接管理
    bool connectToHost(const ModbusConfig& config);  ///< 根据配置建立Modbus连接
    void disconnect();                                ///< 断开连接，停止轮询和心跳
    bool isConnected() const;                         ///< 查询当前是否处于连接状态

    /// @name 寄存器读写
    bool readHoldingRegisters(int serverAddress, int startAddress, int count, QVector<quint16>& values);
    bool readInputRegisters(int serverAddress, int startAddress, int count, QVector<quint16>& values);
    bool writeHoldingRegister(int serverAddress, int address, quint16 value);
    bool writeHoldingRegisters(int serverAddress, int address, const QVector<quint16>& values);

    /// @name 轮询控制
    void setPollConfig(int serverAddress, int startAddress, int count);
    void startPoll();
    void stopPoll();
    bool isPolling() const;
signals:
    void connectionEstablished();                                          ///< 连接建立成功
    void connectionLost();                                                 ///< 连接丢失（心跳检测多次失败）
    void connectionError(const QString& error);                            ///< 连接错误
    void dataReceived(int serverAddress, int startAddress, const QVector<quint16>& values);  ///< 轮询接收到寄存器数据
    void writeCompleted(int serverAddress, int address, bool success);     ///< 写寄存器操作完成
    void heartbeatTimeout();
    void reconnectFailed(const QString& reason);  ///< 重连最终失败（超过最大重试次数）
private:
    void onPollTimeout();
    void onHeartbeatTimeout();
    bool createContext(const ModbusConfig& config);
    void destroyContext();
    void processWriteQueue();
    void attemptReconnect();

    modbus_t* m_ctx = nullptr;              ///< libmodbus 上下文句柄
    ModbusConfig m_config;                  ///< 当前连接配置
    QTimer* m_pollTimer = nullptr;          ///< 轮询定时器
    QTimer* m_heartbeatTimer = nullptr;     ///< 心跳检测定时器
    QAtomicInt m_connected;                 ///< 连接状态标记（原子操作）
    QAtomicInt m_polling;                   ///< 轮询进行中标记（原子操作）
    int m_pollServerAddress = 1;            ///< 轮询目标从站地址
    int m_poolStartAddress = 0;             ///< 轮询起始寄存器地址
    int m_poolCount = 10;                   ///< 轮询寄存器数量
    int m_heartbeatFailCount = 0;           ///< 心跳连续失败计数
    static constexpr int HEARTBEAT_MAX_FAIL = 3;  ///< 最大心跳失败次数阈值
    // @brief 写任务结构体
    struct WriteTask {
        int serverAddress;
        int address;
        QVector<quint16> values;
    };

    QQueue<WriteTask> m_writeQueue;         ///< 写操作队列（FIFO）
    QMutex m_writeMutex;                    ///< 写队列互斥锁
    bool m_writeInProgress = false;         ///< 写操作执行中标记
    QAtomicInt m_reconnecting;              ///< 重连中标记（防止重复重连）
    int m_reconnectAttempts = 0;             ///< 重连尝试计数
    static constexpr int MAX_RECONNECT_ATTEMPTS = 20;  ///< 最大重连次数（约100秒后放弃）

};

#endif // MODBUSCOMM_H
