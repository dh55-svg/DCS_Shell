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
        }
        cfg.fieldbusType = map.value("fieldbus", "modbus").toString();
        cfg.configBasePath = map.value("configPath", "./config").toString();
        return cfg;
    }
};
#endif // APPCONFIG_H
