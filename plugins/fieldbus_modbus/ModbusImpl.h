#ifndef MODBUSIMPL_H
#define MODBUSIMPL_H
#include "IFieldBus.h"
#include "ModbusComm.h"

#include <QVector>
#include <QHash>
#include <memory>
/**
 * @file    ModbusImpl.h
 * @brief   Modbus 协议实现 — IFieldbus 接口的 Modbus 适配器
 *
 * 实现 IFieldbus 接口，管理多台 Modbus 设备的并发通信。
 * 每台设备运行在独立 QThread 中，通过 ModbusComm 进行实际通信，
 * 采集到的数据推送到统一的 IMessageBus 数据总线。
 */
/**
 * @brief Modbus 现场总线实现类
 *
 * 多设备并发管理：每台 Modbus 设备在独立 QThread 中运行，
 * 各线程通过 ModbusComm 轮询寄存器数据，统一推送到消息总线。
 */
class ModbusImpl : public IFieldbus
{
    Q_OBJECT
    Q_INTERFACES(IFieldbus)
public:
    explicit ModbusImpl(QObject *parent = nullptr);
    ~ModbusImpl();
    void addDevice(const DeviceConfig& cfg) override;
    void removeDevice(int devId) override;
    void startDevice(int devId) override;
    void stopDevice(int devId) override;
    void startAll() override;
    void stopAll() override;
    void writeRegister(int devId, int addr, quint16 val) override;
    void setDataSink(IMessageBus* sink) override;
    bool isDeviceConnected(int devId) const override;
    int onlineDeviceCount() const override;
    int totalDeviceCount() const override;
signals:
    void deviceOnline(int id);          ///< 设备上线通知
    void deviceOffline(int id);         ///< 设备下线通知
    void allDevicesOffline();           ///< 所有设备离线通知
private:
    void onDataReceived(int serverAddr, int startAddr, const QVector<quint16>& values);
    void checkAllOffline();

    /// 单台 Modbus 设备的运行上下文（配置 + 通信对象 + 工作线程）
    struct DeviceEntry {
        DeviceConfig config;
        ModbusComm* comm = nullptr;
        QThread* thread = nullptr;
    };
    QVector<DeviceEntry> m_devices;     ///< 所有管理设备的列表
    IMessageBus* m_sink = nullptr;      ///< 数据输出目标总线
    int m_failCount = 0;                ///< 通信失败累计计数
};

#endif // MODBUSIMPL_H
