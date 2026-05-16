// =============================================================================
// QtMqttPlugin.cpp — 真实 QMqttClient 插件实现
// =============================================================================
#include "QtMqttPlugin.h"
#include <QMqttSubscriptionProperties>
#include <QDebug>

QtMqttPlugin::QtMqttPlugin(QObject *parent) : IMqttGateway() {
    Q_UNUSED(parent);
    m_client = new QMqttClient(this);

    // 遗嘱默认 (可通过配置文件覆盖)
    m_client->setWillTopic("dcs/status/" + m_client->clientId());
    m_client->setWillMessage(QByteArray("OFFLINE"));
    m_client->setWillQoS(1);
    m_client->setWillRetain(true);

    connect(m_client, &QMqttClient::connected, this, &QtMqttPlugin::onConnected);
    connect(m_client, &QMqttClient::disconnected, this, &QtMqttPlugin::onDisconnected);
    connect(m_client, &QMqttClient::messageReceived, this, &QtMqttPlugin::onMessageReceived);
    connect(m_client, &QMqttClient::errorChanged, this, &QtMqttPlugin::onError);

    m_reconnectTimer = new QTimer(this);
    m_reconnectTimer->setInterval(5000);
    connect(m_reconnectTimer, &QTimer::timeout, this, &QtMqttPlugin::onReconnect);
}

QtMqttPlugin::~QtMqttPlugin() { disconnectFrom(); }

bool QtMqttPlugin::connectTo(const QString &host, quint16 port) {
    m_host = host;
    m_port = port;
    m_client->setHostname(host);
    m_client->setPort(port);
    m_client->connectToHost();
    return true;
}

void QtMqttPlugin::publish(const QString &topic, const QByteArray &payload, quint8 qos) {
    if (!m_client || m_client->state() != QMqttClient::Connected) return;
    m_client->publish(QMqttTopicName(topic), payload, qos);
}

void QtMqttPlugin::subscribe(const QString &topic, quint8 qos) {
    if (!m_client || m_client->state() != QMqttClient::Connected) {
        m_subscriptions[topic] = qos;  // 缓存, 连上后补订阅
        return;
    }
    m_client->subscribe(QMqttTopicFilter(topic), qos);
    m_subscriptions[topic] = qos;
}

bool QtMqttPlugin::isOnline() const {
    return m_client && m_client->state() == QMqttClient::Connected;
}

void QtMqttPlugin::disconnectFrom() {
    m_reconnectTimer->stop();
    if (m_client) m_client->disconnectFromHost();
}

void QtMqttPlugin::onConnected() {
    m_reconnectTimer->stop();
    // 补订阅
    for (auto it = m_subscriptions.begin(); it != m_subscriptions.end(); ++it)
        m_client->subscribe(QMqttTopicFilter(it.key()), it.value());
    emit onlineChanged(true);
}

void QtMqttPlugin::onDisconnected() {
    emit onlineChanged(false);
    m_reconnectTimer->start();
}

void QtMqttPlugin::onMessageReceived(const QByteArray &msg, const QMqttTopicName &topic) {
    emit messageReceived(topic.name(), msg);
}

void QtMqttPlugin::onError() {
    emit errorOccurred(QString::number(static_cast<int>(m_client->error())));
}

void QtMqttPlugin::onReconnect() {
    if (m_client->state() != QMqttClient::Connected)
        m_client->connectToHost();
}
