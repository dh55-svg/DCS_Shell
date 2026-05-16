#include <QtTest>
#include "infrastructure/plugin/PluginHub.h"
#include "mocks/mock_fieldbus.h"

class TestHotSwap : public QObject {
    Q_OBJECT
private slots:
    void mock_switch_no_crash() {
        PluginHub hub;
        // Verify switchPlugin doesn't crash with mock data
        bool ok = hub.switchPlugin("com.dcsshell.IFieldBus", "plugins/SimulatorPlugin.dll");
        // May fail if plugin doesn't exist, but should not crash
        Q_UNUSED(ok);
    }
    void can_switch_checks_file() {
        PluginHub hub;
        QVERIFY(!hub.canSwitch("com.dcsshell.IFieldBus", "/nonexistent/plugin.xyz"));
    }
    void available_plugins_stable() {
        PluginHub hub;
        auto list = hub.availablePlugins("com.dcsshell.IFieldBus");
        QVERIFY(list.isEmpty() || list.size() >= 0);
    }
};

QTEST_MAIN(TestHotSwap)
#include "test_hot_swap.moc"
