#ifndef IFIELDBUS_H
#define IFIELDBUS_H
#include <QObject>
#include <QVector>
#include <QString>
#include <QJsonObject>

struct DeviceConfig {
    int deviceId = 0;
    int serverAddr = 1;
    QString ip;
    int port = 502;
    int pollIntervalMs = 500;
    int slaveId = 1;
    int regStart = 0;
    int regCount = 128;
};

class IMessageBus;

class IFieldbus : public QObject {
    Q_OBJECT
public:
    explicit IFieldbus(QObject* parent = nullptr) : QObject(parent) {}
    virtual ~IFieldbus() = default;

    virtual QString protocolName() const = 0;
    virtual QString protocolVersion() const = 0;
    virtual QStringList supportedTransports() const = 0;
    virtual QVector<DeviceConfig> allDeviceConfigs() const = 0;
    virtual QJsonObject statusSnapshot() const = 0;

    virtual void addDevice(const DeviceConfig& cfg) = 0;
    virtual void removeDevice(int devId) = 0;
    virtual void startAll() = 0;
    virtual void stopAll() = 0;
    virtual void writeRegister(int devId, int addr, quint16 val) = 0;
    virtual void setDataSink(IMessageBus* sink) = 0;
    virtual bool isDeviceConnected(int devId) const = 0;
    virtual int onlineDeviceCount() const = 0;
    virtual int totalDeviceCount() const = 0;

signals:
    void deviceOnline(int id);
    void deviceOffline(int id);
    void allDevicesOffline();
    void rawDataReceived(int devId, int regAddr, quint16 value);
};

#define IFieldBus_iid "com.dcsshell.IFieldBus"
Q_DECLARE_INTERFACE(IFieldbus, IFieldBus_iid)
#endif
