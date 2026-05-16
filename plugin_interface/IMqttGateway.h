#ifndef IMQTTGATEWAY_H
#define IMQTTGATEWAY_H
#include <QObject>
#include <QByteArray>

class IMqttGateway : public QObject {
    Q_OBJECT
public:
    virtual ~IMqttGateway() = default;
    virtual bool connectTo(const QString& host, quint16 port) = 0;
    virtual void connectEncrypted(const QString& host, quint16 port,
                                   const QString& caCertPath, const QString& clientCertPath,
                                   const QString& clientKeyPath) = 0;
    virtual void publish(const QString& topic, const QByteArray& payload, quint8 qos = 1) = 0;
    virtual void subscribe(const QString& topic, quint8 qos = 1) = 0;
    virtual bool isOnline() const = 0;
    virtual bool isEncrypted() const = 0;
    virtual void disconnectFrom() = 0;

signals:
    void messageReceived(const QString& topic, const QByteArray& payload);
    void onlineChanged(bool online);
    void errorOccurred(const QString& msg);
};

#define IMqttGateway_iid "com.dcsshell.IMqttGateway"
Q_DECLARE_INTERFACE(IMqttGateway, IMqttGateway_iid)
#endif
