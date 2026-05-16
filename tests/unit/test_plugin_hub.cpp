#include <QtTest>
#include <QTemporaryDir>
#include "infrastructure/plugin/PluginHub.h"
#include "infrastructure/nulls/NullFieldBus.h"

class TestPluginHub : public QObject {
    Q_OBJECT
private slots:
    void scan_empty_dir_returns_zero() {
        QTemporaryDir dir;
        PluginHub hub;
        QCOMPARE(hub.scanAll(dir.path()), 0);
    }
    void resolve_null_iid_returns_null() {
        PluginHub hub;
        QVERIFY(hub.resolve<IFieldbus>(nullptr) == nullptr);
    }
    void available_plugins_empty_initially() {
        PluginHub hub;
        QVERIFY(hub.availablePlugins("com.dcsshell.IFieldBus").isEmpty());
    }
    void discover_no_match_returns_empty() {
        PluginHub hub;
        QVERIFY(hub.discover<IFieldbus>("nonexistent.iid").isEmpty());
    }
    void loaded_count_starts_zero() {
        PluginHub hub;
        QCOMPARE(hub.loadedCount(), 0);
    }
    void can_switch_returns_false_for_missing_file() {
        PluginHub hub;
        QVERIFY(!hub.canSwitch("com.dcsshell.IFieldBus", "/nonexistent/plugin.dll"));
    }
};

QTEST_APPLESS_MAIN(TestPluginHub)
#include "test_plugin_hub.moc"
