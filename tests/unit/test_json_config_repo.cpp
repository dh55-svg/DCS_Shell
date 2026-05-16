#include <QtTest>
#include <QTemporaryDir>
#include <QFile>
#include "infrastructure/config/JsonConfigRepo.h"

class TestJsonConfigRepo : public QObject {
    Q_OBJECT
private slots:
    void load_valid_app_config() {
        QTemporaryDir dir;
        QString path = dir.path() + "/app.json";
        QFile f(path);
        f.open(QIODevice::WriteOnly);
        f.write(R"({"dbBackend":"sqlite","fieldbus":"simulator"})");
        f.close();
        JsonConfigRepo repo;
        auto map = repo.loadAppConfig(path);
        QCOMPARE(map.value("dbBackend").toString(), "sqlite");
    }
    void load_missing_file_returns_empty() {
        JsonConfigRepo repo;
        auto map = repo.loadAppConfig("/nonexistent/app.json");
        QVERIFY(map.isEmpty());
    }
    void save_and_load_roundtrip() {
        QTemporaryDir dir;
        QString path = dir.path() + "/tags.json";
        QVariantMap data;
        data["key"] = "value";
        data["num"] = 42;
        JsonConfigRepo repo;
        repo.saveTagsJson(path, data);
        auto loaded = repo.loadTagsJson(path);
        QCOMPARE(loaded.value("key").toString(), "value");
        QCOMPARE(loaded.value("num").toInt(), 42);
    }
};

QTEST_APPLESS_MAIN(TestJsonConfigRepo)
#include "test_json_config_repo.moc"
