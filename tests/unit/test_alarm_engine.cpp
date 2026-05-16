#include <QtTest>
#include "domain/alarm/AlarmEngine.h"
#include "infrastructure/nulls/NullAlarmRepo.h"
#include "infrastructure/nulls/NullHistoryRepo.h"

class TestAlarmEngine : public QObject {
    Q_OBJECT
private:
    NullAlarmRepo* m_alarmRepo = nullptr;
    AlarmEngine* m_engine = nullptr;

private slots:
    void init() {
        m_alarmRepo = new NullAlarmRepo();
        m_engine = new AlarmEngine(*m_alarmRepo, nullptr, nullptr);
        m_engine->initialize();
    }
    void cleanup() {
        delete m_engine;
        delete m_alarmRepo;
    }

    void trigger_alarm_emits_signal() {
        QSignalSpy spy(m_engine, &AlarmEngine::alarmTriggered);
        m_engine->triggerAlarm(101, AlarmLimit::High, 160.0f, 150.0f);
        QCOMPARE(spy.count(), 1);
    }
    void acknowledge_alarm_by_tagId() {
        m_engine->triggerAlarm(101, AlarmLimit::High, 160.0f, 150.0f);
        QVERIFY(m_engine->acknowledgeAlarmByTagId(101, "op"));
    }
    void acknowledge_all() {
        m_engine->triggerAlarm(101, AlarmLimit::High, 160.0f, 150.0f);
        m_engine->triggerAlarm(102, AlarmLimit::Low, 5.0f, 10.0f);
        m_engine->acknowledgeAll("op");
        QCOMPARE(m_engine->unacknowledgedCount(), 0);
    }
    void shelve_alarm() {
        m_engine->triggerAlarm(101, AlarmLimit::High, 160.0f, 150.0f);
        m_engine->shelveAlarm(101, "test", 300);
        QCOMPARE(m_engine->shelveCount(101), 1);
    }
    void unshelve_alarm() {
        m_engine->triggerAlarm(101, AlarmLimit::High, 160.0f, 150.0f);
        m_engine->shelveAlarm(101, "test", 300);
        m_engine->unshelveAlarm(101);
        QCOMPARE(m_engine->shelveCount(101), 1);
    }
    void active_alarm_count() {
        m_engine->triggerAlarm(101, AlarmLimit::High, 160.0f, 150.0f);
        m_engine->triggerAlarm(102, AlarmLimit::Low, 5.0f, 10.0f);
        QCOMPARE(m_engine->activeAlarmCount(), 2);
    }
    void sound_enabled_toggle() {
        m_engine->setSoundEnabled(false);
        QVERIFY(!m_engine->soundEnabled());
        m_engine->setSoundEnabled(true);
        QVERIFY(m_engine->soundEnabled());
    }
    void kpi_monitor_accessible() {
        QVERIFY(m_engine->kpiMonitor() != nullptr);
    }
    void change_log_accessible() {
        QVERIFY(m_engine->changeLog() != nullptr);
    }
};

QTEST_APPLESS_MAIN(TestAlarmEngine)
#include "test_alarm_engine.moc"
