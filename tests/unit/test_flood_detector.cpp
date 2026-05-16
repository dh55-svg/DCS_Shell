#include <QtTest>
#include "domain/alarm/FloodDetector.h"

class TestFloodDetector : public QObject {
    Q_OBJECT
private slots:
    void no_flood_with_few_alarms() {
        FloodDetector fd;
        for (int i = 0; i < 5; ++i)
            fd.recordAlarm(100 + i, "TAG", AlarmPriority::Major);
        QVERIFY(!fd.isInFlood());
    }
    void flood_detected_after_threshold() {
        FloodDetector fd;
        for (int i = 0; i < 12; ++i)
            fd.recordAlarm(200 + i, "TAG", AlarmPriority::Major);
        QVERIFY(fd.isInFlood());
    }
    void critical_priority_has_higher_weight() {
        FloodDetector fd;
        for (int i = 0; i < 3; ++i)
            fd.recordAlarm(300 + i, "CRIT", AlarmPriority::Critical);
        // 3 critical = 15 weighted, exceeds 10 threshold
        QVERIFY(fd.weightedCount() >= 15);
    }
    void minor_priority_has_lower_weight() {
        FloodDetector fd;
        for (int i = 0; i < 5; ++i)
            fd.recordAlarm(400 + i, "MINOR", AlarmPriority::Advisory);
        QVERIFY(!fd.isInFlood());
        QVERIFY(fd.weightedCount() <= 5);
    }
};

QTEST_MAIN(TestFloodDetector)
#include "test_flood_detector.moc"
