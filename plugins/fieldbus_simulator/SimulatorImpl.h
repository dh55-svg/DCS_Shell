#ifndef SIMULATORIMPL_H
#define SIMULATORIMPL_H

#include <QTimer>
#include <QVector>
#include <QHash>
#include <QtMath>
#include "plugin_interface/IFieldBus.h"
class SimulatorImpl : public IFieldbus
{
    Q_OBJECT
    Q_INTERFACES(IFieldbus)
public:
    explicit SimulatorImpl(QObject *parent = nullptr);
    ~SimulatorImpl();

    QString protocolName() const override { return "Simulator"; }
    QString protocolVersion() const override { return "1.0.0"; }
    QStringList supportedTransports() const override { return {"virtual"}; }
    QVector<DeviceConfig> allDeviceConfigs() const override;
    QJsonObject statusSnapshot() const override;

    void addDevice(const DeviceConfig& cfg) override;
    void removeDevice(int devId) override;
    void startAll() override;
    void stopAll() override;
    void writeRegister(int devId, int addr, quint16 val) override;
    void setDataSink(IMessageBus* sink) override;
    bool isDeviceConnected(int devId) const override;
    int onlineDeviceCount() const override;
    int totalDeviceCount() const override;
private:
    void onTick();  ///< 定时器回调：生成仿真数据并推送到消息总线

    /// 仿真设备上下文
    struct SimDevice {
        DeviceConfig config;                      ///< 设备配置
        QVector<QPair<int, float>> tagPhases;     ///< 每个寄存器对应的正弦波相位偏移
        bool running = false;                     ///< 运行状态
    };
    QVector<SimDevice> m_devices;    ///< 仿真设备列表
    QTimer m_timer;                  ///< 定时器，控制数据生成频率
    IMessageBus* m_sink = nullptr;   ///< 数据输出目标总线
    int m_tick = 0;                  ///< 定时器滴答计数（用于正弦波相位计算）
};

#endif // SIMULATORIMPL_H
