#ifndef APPCONFIG_H
#define APPCONFIG_H
#include <QString>
#include <QVariantMap>
#include "../infrastructure/config/IConfigRepo.h"
#include "../infrastructure/security/CryptoConfig.h"
struct AppConfig{
    QString dbBackend="mysql";
    struct MysqlConfig{
        QString host="127.0.0.1";
        int port = 3306;
        QString database = "dcs";
        QString user = "root";
        QString password = "";
        int poolSize = 5;
    }mysql;
    struct SqliteConfig {
        QString path = "data/dcs.db";
    } sqlite;

    struct MqttConfig {
        QString host = "127.0.0.1";
        int port = 1883;
        QString clientId;
        bool enabled = false;
        quint16 keepAlive = 60;           // MQTT 协议心跳 (秒)
        bool cleanSession = true;
        int reconnectBaseMs = 5000;       // 重连基础间隔 (毫秒)
        int reconnectMaxMs  = 60000;      // 重连最大间隔 (毫秒)
        int heartbeatInterval = 30;       // 应用层心跳周期 (秒), 0=禁用
        struct TlsConfig {
            bool enabled = false;
            QString caCertPath;
            QString clientCertPath;
            QString clientKeyPath;
        } tls;
    } mqtt;

    QString fieldbusType = "modbus"; // "modbus", "simulator", "opcua"
    QString configBasePath = "./config";
    struct AppConfig fromJson(const QString& path, IConfigRepo& repo){
        AppConfig cfg;
        auto map=repo.loadAppConfig(path);
        cfg.dbBackend = map.value("dbBackend", "sqlite").toString();
        QVariantMap m = map.value("mysql").toMap();
        if (!m.isEmpty()) {
            cfg.mysql.host = m.value("host", "127.0.0.1").toString();
            cfg.mysql.port = m.value("port", 3306).toInt();
            cfg.mysql.database = m.value("database", "dcs").toString();
            cfg.mysql.user = m.value("user", "root").toString();
            cfg.mysql.password = m.value("password", "").toString();
            // 若为 ENC: 前缀密文则自动解密
            if (cfg.mysql.password.startsWith("ENC:")) {
                cfg.mysql.password = CryptoConfig::decryptPassword(cfg.mysql.password);
            }
        }
        QVariantMap mq = map.value("mqtt").toMap();
        if (!mq.isEmpty()) {
            cfg.mqtt.host = mq.value("host", "127.0.0.1").toString();
            cfg.mqtt.port = mq.value("port", 1883).toInt();
            cfg.mqtt.clientId = mq.value("clientId", "").toString();
            cfg.mqtt.enabled = mq.value("enabled", false).toBool();
            cfg.mqtt.keepAlive = mq.value("keepAlive", 60).toInt();
            cfg.mqtt.cleanSession = mq.value("cleanSession", true).toBool();
            cfg.mqtt.reconnectBaseMs = mq.value("reconnectBaseMs", 5000).toInt();
            cfg.mqtt.reconnectMaxMs = mq.value("reconnectMaxMs", 60000).toInt();
            cfg.mqtt.heartbeatInterval = mq.value("heartbeatInterval", 30).toInt();
            QVariantMap tls = mq.value("tls").toMap();
            if (!tls.isEmpty()) {
                cfg.mqtt.tls.enabled = tls.value("enabled", false).toBool();
                cfg.mqtt.tls.caCertPath = tls.value("caCertPath", "").toString();
                cfg.mqtt.tls.clientCertPath = tls.value("clientCertPath", "").toString();
                cfg.mqtt.tls.clientKeyPath = tls.value("clientKeyPath", "").toString();
            }
        }
        cfg.fieldbusType = map.value("fieldbus", "modbus").toString();
        cfg.configBasePath = map.value("configPath", "./config").toString();
        return cfg;
    }
};
#endif // APPCONFIG_H
