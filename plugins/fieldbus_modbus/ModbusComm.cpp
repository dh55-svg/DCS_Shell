#include "ModbusComm.h"
#include <modbus.h>
#include <QDateTime>
#include <QDebug>
#include <QTimer>
// 内部日志辅助函数
static void logMsg(const QString& level, const QString& msg) {
    qDebug().noquote() << QString("[%1][ModbusComm] %2").arg(level, msg);
}
ModbusComm::ModbusComm(QObject *parent)
    : QObject{parent}
{
    m_connected.storeRelaxed(0);
    m_polling.storeRelaxed(0);
    m_reconnecting.storeRelaxed(0);

    m_pollTimer=new QTimer(this);
    m_pollTimer->setSingleShot(false);
    connect(m_pollTimer, &QTimer::timeout, this, &ModbusComm::onPollTimeout);

    m_heartbeatTimer = new QTimer(this);
    m_heartbeatTimer->setSingleShot(false);
    connect(m_heartbeatTimer, &QTimer::timeout, this, &ModbusComm::onHeartbeatTimeout);

    logMsg("INFO", "Modbus通讯模块初始化完成");
}

ModbusComm::~ModbusComm()
{
    stopPoll();
    disconnect();
}

bool ModbusComm::connectToHost(const ModbusConfig &config)
{
    m_config=config;
    if(!createContext(config))
    {
        emit connectionError(QString("创建Modbus上下文失败"));
        return false;
    }
    if(modbus_connect(m_ctx)==-1)
    {
        QString error = QString::fromUtf8(modbus_strerror(errno));
        logMsg("ERROR", QString("连接失败: %1").arg(error));
        emit connectionError(error);
        destroyContext();
        return false;
    }
    m_connected.storeRelaxed(1);
    m_heartbeatFailCount=0;
    m_heartbeatTimer->start(config.heartbeatInterval);
    emit connectionEstablished();
    logMsg("INFO", "Modbus连接已建立");
    return true;
}

void ModbusComm::disconnect()
{
    stopPoll();
    m_heartbeatTimer->stop();
    m_connected.storeRelaxed(0);
    m_reconnecting.storeRelaxed(0);
    destroyContext();
}

bool ModbusComm::isConnected() const
{
    return m_connected.loadRelaxed()!=0;
}

bool ModbusComm::readHoldingRegisters(int serverAddress, int startAddress, int count, QVector<quint16> &values)
{
    if(!isConnected()||!m_ctx) return false;
    if(modbus_set_slave(m_ctx,serverAddress)==-1)
    {
        logMsg("WARN", QString("设置从站地址失败: %1").arg(serverAddress));
        return false;
    }
    values.resize(count);

    int rc=modbus_read_registers(m_ctx,startAddress,count,values.data());
    if (rc == -1) {
        logMsg("WARN", QString("读取保持寄存器失败: 从站=%1, 起始=%2, 数量=%3, 错误=%4")
                           .arg(serverAddress).arg(startAddress).arg(count)
                           .arg(modbus_strerror(errno)));
        return false;
    }
    return true;
}

bool ModbusComm::readInputRegisters(int serverAddress, int startAddress, int count, QVector<quint16> &values)
{
    if (!isConnected() || !m_ctx) return false;
    if (modbus_set_slave(m_ctx, serverAddress) == -1) return false;
    values.resize(count);
    int rc = modbus_read_input_registers(m_ctx, startAddress, count, values.data());
    if (rc == -1) {
        logMsg("WARN", QString("读取输入寄存器失败: 从站=%1, 起始=%2, 数量=%3")
                           .arg(serverAddress).arg(startAddress).arg(count));
        return false;
    }
    return true;
}

bool ModbusComm::writeHoldingRegister(int serverAddress, int address, quint16 value)
{
    return writeHoldingRegisters(serverAddress, address, {value});
}

bool ModbusComm::writeHoldingRegisters(int serverAddress, int address, const QVector<quint16> &values)
{
    QMutexLocker lock(&m_writeMutex);
    WriteTask task;
    task.serverAddress = serverAddress;
    task.address = address;
    task.values = values;
    m_writeQueue.enqueue(task);
    if (!m_writeInProgress) {
        m_writeInProgress = true;
        lock.unlock();
        processWriteQueue();
    }
    return true;
}

void ModbusComm::setPollConfig(int serverAddress, int startAddress, int count)
{
    m_pollServerAddress = serverAddress;
    m_poolStartAddress = startAddress;
    m_poolCount = count;
}

void ModbusComm::startPoll()
{
    if (m_polling.loadRelaxed()) return;
    m_polling.storeRelaxed(1);
    m_pollTimer->start(m_config.poolInterval);
}

void ModbusComm::stopPoll()
{
    m_polling.storeRelaxed(0);
    m_pollTimer->stop();
}

bool ModbusComm::isPolling() const
{
    return m_polling.loadRelaxed() != 0;
}

void ModbusComm::onPollTimeout()
{
    if (!isConnected() || !m_ctx) return;
    QVector<quint16> values;
    if (readHoldingRegisters(m_pollServerAddress, m_poolStartAddress, m_poolCount, values)) {
        emit dataReceived(m_pollServerAddress, m_poolStartAddress, values);
    }
}

void ModbusComm::onHeartbeatTimeout()
{
    if (!m_ctx) return;
    QVector<quint16> values;
    bool ok = readHoldingRegisters(m_pollServerAddress, m_poolStartAddress, m_poolCount, values);
    if (!ok) {
        m_heartbeatFailCount++;
        logMsg("WARN", QString("心跳检测失败 (%1/%2)").arg(m_heartbeatFailCount).arg(HEARTBEAT_MAX_FAIL));
        if (m_heartbeatFailCount >= HEARTBEAT_MAX_FAIL) {
            m_connected.storeRelaxed(0);
            stopPoll();
            emit connectionLost();
            logMsg("ERROR", "通信中断，准备自动重连...");
            attemptReconnect();
        }
    } else {
        m_heartbeatFailCount = 0;
    }
}

bool ModbusComm::createContext(const ModbusConfig &config)
{
    destroyContext();
    // ★ 修复悬空指针：toUtf8() 返回临时 QByteArray，必须提升为局部变量保持生命周期
    if(config.type==ModbusConfig::Tcp)
    {
        QByteArray hostBytes = config.host.toUtf8();
        m_ctx = modbus_new_tcp(hostBytes.constData(), config.port);
        if (!m_ctx) {
            logMsg("ERROR", QString("创建TCP上下文失败: %1").arg(modbus_strerror(errno)));
            return false;
        }
    }else{
        QByteArray portBytes = config.portName.toUtf8();
        m_ctx = modbus_new_rtu(portBytes.constData(), config.baudRate, config.parity, config.dataBit, config.stopBit);
        if (!m_ctx) {
            logMsg("ERROR", QString("创建RTU上下文失败: %1").arg(modbus_strerror(errno)));
            return false;
        }
    }
    uint32_t toSec = config.timeout / 1000;
    uint32_t toUSec = (config.timeout % 1000) * 1000;
    modbus_set_response_timeout(m_ctx, toSec, toUSec);
    modbus_set_byte_timeout(m_ctx, toSec / 2, toUSec / 2);
    modbus_set_error_recovery(m_ctx,
                              static_cast<modbus_error_recovery_mode>(MODBUS_ERROR_RECOVERY_LINK | MODBUS_ERROR_RECOVERY_PROTOCOL));
    return true;

}

void ModbusComm::destroyContext()
{
    if (m_ctx) {
        modbus_close(m_ctx);
        modbus_free(m_ctx);
        m_ctx = nullptr;
    }
}
//处理Modbus写操作队列，批量执行待处理的写任务。s
void ModbusComm::processWriteQueue()
{
    if (!isConnected() || !m_ctx) {
        QMutexLocker lock(&m_writeMutex);
        m_writeInProgress = false;
        return;
    }
    while (true) {
        QMutexLocker lock(&m_writeMutex);
        if (m_writeQueue.isEmpty()) {
            m_writeInProgress = false;
            return;
        }
        WriteTask task = m_writeQueue.dequeue();
        lock.unlock();

        if (modbus_set_slave(m_ctx, task.serverAddress) == -1) {
            emit writeCompleted(task.serverAddress, task.address, false);
            continue;
        }
        int rc = modbus_write_registers(m_ctx, task.address, task.values.size(),
                                        reinterpret_cast<const uint16_t*>(task.values.constData()));
        if (rc == -1) {
            logMsg("ERROR", QString("写队列写入失败: 从站=%1, 地址=%2, 错误=%3")
                                .arg(task.serverAddress).arg(task.address)
                                .arg(QString::fromUtf8(modbus_strerror(errno))));
            emit writeCompleted(task.serverAddress, task.address, false);
            // ★ 修复队列残留：continue 处理下一任务，而非 return 放弃整个队列
            continue;
        }
        emit writeCompleted(task.serverAddress, task.address, true);
    }
}
//实现Modbus连接的自动重连机制
void ModbusComm::attemptReconnect()
{
    if (m_reconnecting.loadRelaxed()) return;
    m_reconnecting.storeRelaxed(1);
    m_reconnectAttempts++;
    // ★ 最大重试次数限制，超限后放弃并通知上层
    if (m_reconnectAttempts > MAX_RECONNECT_ATTEMPTS) {
        m_reconnecting.storeRelaxed(0);
        logMsg("ERROR", QString("重连失败已达上限(%1次)，停止重试").arg(MAX_RECONNECT_ATTEMPTS));
        emit reconnectFailed(QString("超过最大重试次数(%1)").arg(MAX_RECONNECT_ATTEMPTS));
        return;
    }
    QTimer::singleShot(2000, this, [this]() {
        logMsg("INFO", QString("正在重连... (第%1/%2次)").arg(m_reconnectAttempts).arg(MAX_RECONNECT_ATTEMPTS));
        destroyContext();
        if (connectToHost(m_config)) {
            m_reconnecting.storeRelaxed(0);
            m_reconnectAttempts = 0;  // 成功后重置计数
            logMsg("INFO", "重连成功，自动开始轮询");
            startPoll();
        } else {
            m_reconnecting.storeRelaxed(0);
            logMsg("ERROR", "重连失败，5秒后再次尝试");
            QTimer::singleShot(5000, this, &ModbusComm::attemptReconnect);
        }
    });
}
