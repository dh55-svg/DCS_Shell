// =============================================================================
// QtMqttPlugin.h — MQTT 插件 (真实 QMqttClient 封装)
// =============================================================================
#ifndef QTMQTTPLUGIN_H
#define QTMQTTPLUGIN_H
#include "plugin_interface/IMqttGateway.h"
#include <QMqttClient>
#include <QTimer>

class QtMqttPlugin : public IMqttGateway {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID IMqttGateway_iid FILE "QtMqttPlugin.json")
    Q_INTERFACES(IMqttGateway)

public:
    explicit QtMqttPlugin(QObject *parent = nullptr);
    ~QtMqttPlugin() override;

    bool connectTo(const QString &host, quint16 port) override;
    void publish(const QString &topic, const QByteArray &payload, quint8 qos) override;
    void subscribe(const QString &topic, quint8 qos) override;
    bool isOnline() const override;
    void disconnectFrom() override;

private slots:
    void onConnected();
    void onDisconnected();
    void onMessageReceived(const QByteArray &msg, const QMqttTopicName &topic);
    void onError();
    void onReconnect();

private:
    QMqttClient *m_client;
    QTimer *m_reconnectTimer;
    QString m_host;
    quint16 m_port = 1883;
    QHash<QString, quint8> m_subscriptions;
};

#endif
