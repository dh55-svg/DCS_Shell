#ifndef NULLFIELDBUS_H
#define NULLFIELDBUS_H
#include "plugin_interface/IFieldBus.h"

class NullFieldbus : public IFieldbus {
    Q_OBJECT
    Q_INTERFACES(IFieldbus)
public:
    using IFieldbus::IFieldbus;

    QString protocolName() const override { return "None"; }
    QString protocolVersion() const override { return "0.0.0"; }
    QStringList supportedTransports() const override { return {}; }
    QVector<DeviceConfig> allDeviceConfigs() const override { return {}; }
    QJsonObject statusSnapshot() const override { return {}; }
    void addDevice(const DeviceConfig&) override {}
    void removeDevice(int) override {}
    void startAll() override { emit allDevicesOffline(); }
    void stopAll() override {}
    void writeRegister(int, int, quint16) override {}
    void setDataSink(IMessageBus*) override {}
    bool isDeviceConnected(int) const override { return false; }
    int onlineDeviceCount() const override { return 0; }
    int totalDeviceCount() const override { return 0; }
};
#endif
