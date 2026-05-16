#include <QtTest>
#include <QTemporaryDir>
#include <QFile>
#include "app/AppConfig.h"
#include "infrastructure/config/JsonConfigRepo.h"

class TestAppConfig : public QObject {
    Q_OBJECT
private slots:
    void default_values() {
        JsonConfigRepo repo;
        AppConfig cfg;
        QCOMPARE(cfg.dbBackend, "sqlite");
        QCOMPARE(cfg.fieldbusType, "modbus");
        QCOMPARE(cfg.mqtt.enabled, false);
    }
    void parse_from_json() {
        QTemporaryDir dir;
        QString path = dir.path() + "/app.json";
        QFile f(path);
        f.open(QIODevice::WriteOnly);
        f.write(R"({"dbBackend":"mysql","fieldbus":"simulator","mqtt":{"enabled":true,"host":"10.0.0.1","port":1884}})");
        f.close();

        JsonConfigRepo repo;
        AppConfig cfg = AppConfig{}.fromJson(path, repo);
        QCOMPARE(cfg.dbBackend, "mysql");
        QCOMPARE(cfg.fieldbusType, "simulator");
        QVERIFY(cfg.mqtt.enabled);
        QCOMPARE(cfg.mqtt.host, "10.0.0.1");
        QCOMPARE(cfg.mqtt.port, 1884);
    }
    void missing_file_uses_defaults() {
        JsonConfigRepo repo;
        AppConfig cfg = AppConfig{}.fromJson("/nonexistent/app.json", repo);
        QCOMPARE(cfg.dbBackend, "sqlite");
    }
};

QTEST_APPLESS_MAIN(TestAppConfig)
#include "test_app_config.moc"
