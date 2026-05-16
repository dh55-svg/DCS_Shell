#include <QtTest>
#include "domain/tag/DeadbandFilter.h"

class TestDeadbandFilter : public QObject {
    Q_OBJECT
private slots:
    void high_alarm_triggers_when_above_threshold_plus_deadband() {
        QVERIFY(DeadbandFilter::exceedsDeadbaud(95.0f, 80.0f, 5.0f, AlarmLimit::High, 70.0f));
    }
    void high_alarm_no_trigger_when_below_threshold() {
        QVERIFY(!DeadbandFilter::exceedsDeadbaud(82.0f, 80.0f, 5.0f, AlarmLimit::High, 70.0f));
    }
    void high_alarm_triggers_on_crossing_threshold() {
        QVERIFY(DeadbandFilter::exceedsDeadbaud(85.0f, 80.0f, 5.0f, AlarmLimit::High, 79.0f));
    }
    void low_alarm_triggers_when_below_threshold_minus_deadband() {
        QVERIFY(DeadbandFilter::exceedsDeadbaud(10.0f, 20.0f, 5.0f, AlarmLimit::Low, 30.0f));
    }
    void low_alarm_no_trigger_when_above_threshold() {
        QVERIFY(!DeadbandFilter::exceedsDeadbaud(22.0f, 20.0f, 5.0f, AlarmLimit::Low, 30.0f));
    }
    void high_returns_to_normal_within_deadband() {
        QVERIFY(DeadbandFilter::returnsToNormal(75.0f, 80.0f, 5.0f, AlarmLimit::High));
    }
    void high_not_returned_to_normal_outside_deadband() {
        QVERIFY(!DeadbandFilter::returnsToNormal(78.0f, 80.0f, 5.0f, AlarmLimit::High));
    }
    void low_returns_to_normal_above_threshold_plus_deadband() {
        QVERIFY(DeadbandFilter::returnsToNormal(27.0f, 20.0f, 5.0f, AlarmLimit::Low));
    }
};

QTEST_MAIN(TestDeadbandFilter)
#include "test_deadband_filter.moc"
