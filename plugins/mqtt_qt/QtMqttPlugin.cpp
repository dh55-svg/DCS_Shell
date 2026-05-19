// =============================================================================
// QtMqttPlugin.cpp — 真实 QMqttClient 插件实现
// =============================================================================
#include "QtMqttPlugin.h"
#include <QMqttSubscriptionProperties>
#include <QSslConfiguration>
#include <QSslCertificate>
#include <QSslKey>
#include <QSslSocket>
#include <QFile>
#include <QDebug>

QtMqttPlugin::QtMqttPlugin(QObject *parent) : IMqttGateway() {
    Q_UNUSED(parent);
    m_client = new QMqttClient(this);

    // 遗嘱: 异常断开时 broker 自动发布 OFFLINE
    m_client->setWillTopic("dcs/status/" + m_client->clientId());
    m_client->setWillMessage(QByteArray("OFFLINE"));
    m_client->setWillQoS(1);
    m_client->setWillRetain(true);

    connect(m_client, &QMqttClient::connected, this, &QtMqttPlugin::onConnected);
    connect(m_client, &QMqttClient::disconnected, this, &QtMqttPlugin::onDisconnected);
    connect(m_client, &QMqttClient::messageReceived, this, &QtMqttPlugin::onMessageReceived);
    connect(m_client, &QMqttClient::errorChanged, this, &QtMqttPlugin::onError);

    // 重连定时器 (指数退避, 初始间隔由 setReconnectInterval 控制)
    m_reconnectTimer = new QTimer(this);
    m_reconnectTimer->setSingleShot(true);
    connect(m_reconnectTimer, &QTimer::timeout, this, &QtMqttPlugin::onReconnect);

    // 应用层心跳定时器
    m_heartbeatTimer = new QTimer(this);
    connect(m_heartbeatTimer, &QTimer::timeout, this, &QtMqttPlugin::onHeartbeat);
}

QtMqttPlugin::~QtMqttPlugin() { disconnectFrom(); }

// ---------------------------------------------------------------------------
// 配置接口
// ---------------------------------------------------------------------------

void QtMqttPlugin::setKeepAlive(quint16 secs) {
    m_client->setKeepAlive(secs);
}

void QtMqttPlugin::setReconnectInterval(int baseMs, int maxMs) {
    m_reconnectBaseMs = qMax(1000, baseMs);
    m_reconnectMaxMs  = qMax(m_reconnectBaseMs, maxMs);
}

void QtMqttPlugin::setHeartbeatInterval(int secs) {
    if (secs > 0) {
        m_heartbeatTimer->setInterval(secs * 1000);
    } else {
        m_heartbeatTimer->stop();
    }
}

// ---------------------------------------------------------------------------
// 连接 / 断开
// ---------------------------------------------------------------------------

bool QtMqttPlugin::connectTo(const QString &host, quint16 port) {
    m_host = host;
    m_port = port;
    m_encrypted = false;
    m_client->setHostname(host);
    m_client->setPort(port);
    m_client->connectToHost();
    return true;
}

void QtMqttPlugin::connectEncrypted(const QString &host, quint16 port,
                                     const QString &caCertPath, const QString &clientCertPath,
                                     const QString &clientKeyPath) {
    m_host = host;
    m_port = port;
    m_encrypted = true;

    m_sslConfig = QSslConfiguration::defaultConfiguration();
    if (!caCertPath.isEmpty()) {
        QList<QSslCertificate> caCerts = QSslCertificate::fromPath(caCertPath, QSsl::Pem);
        if (!caCerts.isEmpty())
            m_sslConfig.setCaCertificates(caCerts);
    }
    if (!clientCertPath.isEmpty() && !clientKeyPath.isEmpty()) {
        QFile certFile(clientCertPath);
        QFile keyFile(clientKeyPath);
        if (certFile.open(QIODevice::ReadOnly) && keyFile.open(QIODevice::ReadOnly)) {
            QSslCertificate clientCert(&certFile, QSsl::Pem);
            QSslKey clientKey(&keyFile, QSsl::Rsa, QSsl::Pem);
            m_sslConfig.setLocalCertificate(clientCert);
            m_sslConfig.setPrivateKey(clientKey);
            certFile.close();
            keyFile.close();
        }
    }
    m_sslConfig.setPeerVerifyMode(QSslSocket::VerifyPeer);

    m_client->setHostname(host);
    m_client->setPort(port);
    m_client->connectToHostEncrypted(m_sslConfig);
}

void QtMqttPlugin::disconnectFrom() {
    m_reconnectTimer->stop();
    m_heartbeatTimer->stop();
    m_reconnectAttempt = 0;
    if (m_client) m_client->disconnectFromHost();
}

// ---------------------------------------------------------------------------
// 发布 / 订阅
// ---------------------------------------------------------------------------

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

// ---------------------------------------------------------------------------
// 槽函数
// ---------------------------------------------------------------------------

void QtMqttPlugin::onConnected() {
    m_reconnectTimer->stop();
    m_reconnectAttempt = 0;

    // 补订阅
    for (auto it = m_subscriptions.begin(); it != m_subscriptions.end(); ++it)
        m_client->subscribe(QMqttTopicFilter(it.key()), it.value());

    // 发布 ONLINE 状态 (与遗嘱对应)
    m_client->publish(QMqttTopicName("dcs/status/" + m_client->clientId()),
                      QByteArray("ONLINE"), 1, true);

    // 启动应用层心跳
    if (m_heartbeatTimer->interval() > 0)
        m_heartbeatTimer->start();

    emit onlineChanged(true);
}

void QtMqttPlugin::onDisconnected() {
    m_heartbeatTimer->stop();
    emit onlineChanged(false);

    // 指数退避: base * 2^attempt, 上限 max
    int delay = qMin(m_reconnectBaseMs * (1 << m_reconnectAttempt), m_reconnectMaxMs);
    m_reconnectAttempt++;
    m_reconnectTimer->setInterval(delay);
    m_reconnectTimer->start();

    qDebug() << "[MQTT] 断开连接, 将在" << delay << "ms 后重连 (第"
             << m_reconnectAttempt << "次)";
}

void QtMqttPlugin::onMessageReceived(const QByteArray &msg, const QMqttTopicName &topic) {
    emit messageReceived(topic.name(), msg);
}

void QtMqttPlugin::onError() {
    emit errorOccurred(QString::number(static_cast<int>(m_client->error())));
}

void QtMqttPlugin::onReconnect() {
    if (m_client->state() == QMqttClient::Connected) return;
    attemptConnect();
}

void QtMqttPlugin::onHeartbeat() {
    if (!isOnline()) return;
    m_client->publish(QMqttTopicName("dcs/status/" + m_client->clientId()),
                      QByteArray("ONLINE"), 1, true);
}

// ---------------------------------------------------------------------------
// 内部: 根据是否 TLS 选择连接方式
// ---------------------------------------------------------------------------

void QtMqttPlugin::attemptConnect() {
    m_client->setHostname(m_host);
    m_client->setPort(m_port);
    if (m_encrypted) {
        m_client->connectToHostEncrypted(m_sslConfig);
    } else {
        m_client->connectToHost();
    }
}
