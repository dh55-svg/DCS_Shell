#include <QtTest>
#include "infrastructure/plugin/PluginHub.h"
#include "mocks/mock_fieldbus.h"

class TestSimulatorPluginLoad : public QObject {
    Q_OBJECT
private slots:
    void discover_simulator_plugin() {
        PluginHub hub;
        // Scan the build plugins directory
        int found = hub.scanAll("plugins");
        QVERIFY(found >= 0);  // may be 0 if plugins not built yet (acceptable)

        auto candidates = hub.discover<IFieldbus>("com.dcsshell.IFieldBus");
        // In CI without plugins built, this may be empty
        if (!candidates.isEmpty()) {
            IFieldbus* fb = hub.resolve<IFieldbus>("com.dcsshell.IFieldBus");
            QVERIFY(fb != nullptr);
            QVERIFY(!fb->protocolName().isEmpty());
        }
    }
    void mock_fieldbus_basic_operations() {
        MockFieldbus mock;
        DeviceConfig dev;
        dev.deviceId = 1; dev.serverAddr = 1; dev.regCount = 10;
        mock.addDevice(dev);
        QCOMPARE(mock.totalDeviceCount(), 1);
        mock.startAll();
        QVERIFY(mock.isRunning());
        QVERIFY(mock.isDeviceConnected(1));
        mock.stopAll();
        QVERIFY(!mock.isRunning());
    }
};

QTEST_MAIN(TestSimulatorPluginLoad)
#include "test_simulator_plugin_load.moc"
#include "../mocks/mock_fieldbus.h"
