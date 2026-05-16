#ifndef MOCK_FIELDBUS_H
#define MOCK_FIELDBUS_H
#include "plugin_interface/IFieldBus.h"

class MockFieldbus : public IFieldbus {
    Q_OBJECT
    Q_INTERFACES(IFieldbus)
public:
    using IFieldbus::IFieldbus;

    QString protocolName() const override { return "Mock"; }
    QString protocolVersion() const override { return "1.0.0"; }
    QStringList supportedTransports() const override { return {"mock"}; }
    QVector<DeviceConfig> allDeviceConfigs() const override { return m_devices; }
    QJsonObject statusSnapshot() const override { return {}; }

    void addDevice(const DeviceConfig& cfg) override { m_devices.append(cfg); }
    void removeDevice(int devId) override {
        m_devices.erase(std::remove_if(m_devices.begin(), m_devices.end(),
            [devId](const DeviceConfig& d) { return d.deviceId == devId; }), m_devices.end());
    }
    void startAll() override { m_running = true; }
    void stopAll() override { m_running = false; }
    void writeRegister(int, int, quint16) override { m_writeCount++; }
    void setDataSink(IMessageBus*) override {}
    bool isDeviceConnected(int devId) const override {
        for (auto& d : m_devices) if (d.deviceId == devId) return m_running;
        return false;
    }
    int onlineDeviceCount() const override { return m_running ? m_devices.size() : 0; }
    int totalDeviceCount() const override { return m_devices.size(); }

    bool isRunning() const { return m_running; }
    int writeCount() const { return m_writeCount; }

    void simulateDeviceOnline(int id) { emit deviceOnline(id); }
    void simulateDeviceOffline(int id) { emit deviceOffline(id); }

private:
    QVector<DeviceConfig> m_devices;
    bool m_running = false;
    int m_writeCount = 0;
};
#endif
