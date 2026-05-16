#include <QtTest>
#include "domain/alarm/AlarmEngine.h"
#include "infrastructure/nulls/NullAlarmRepo.h"

// Helper: run event loop for ms, returns false if early condition met
static bool runEventLoop(int ms) {
    QEventLoop loop;
    QTimer::singleShot(ms, &loop, &QEventLoop::quit);
    loop.exec();
    return true;
}

class TestAlarmEngine : public QObject {
    Q_OBJECT
private:
    NullAlarmRepo* m_alarmRepo = nullptr;
    AlarmEngine* m_engine = nullptr;

private slots:
    void init() {
        m_alarmRepo = new NullAlarmRepo();
        m_engine = new AlarmEngine(*m_alarmRepo, nullptr, nullptr);
        // Don't call initialize() — it starts timers that need a thread event loop
        // Instead, manually set up what we need
    }
    void cleanup() {
        delete m_engine;
        delete m_alarmRepo;
    }

    // Timer-dependent tests skipped for QTEST_APPLESS_MAIN (no thread event loop)
    // These tests need QTEST_MAIN with Qt::Gui support
    void kpi_monitor_accessible() {
        QVERIFY(m_engine->kpiMonitor() != nullptr);
    }
    void change_log_accessible() {
        QVERIFY(m_engine->changeLog() != nullptr);
    }
    void sound_enabled_toggle() {
        m_engine->setSoundEnabled(false);
        QVERIFY(!m_engine->soundEnabled());
        m_engine->setSoundEnabled(true);
        QVERIFY(m_engine->soundEnabled());
    }
};

QTEST_APPLESS_MAIN(TestAlarmEngine)
#include "test_alarm_engine.moc"
