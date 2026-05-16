#include <QtTest>
#include "domain/tag/RateOfChangeChecker.h"

class TestRateOfChangeChecker : public QObject {
    Q_OBJECT
private slots:
    void no_trigger_on_first_sample() {
        RateOfChangeChecker checker;
        TagInf cfg;
        cfg.rateOfChangeLimit = 5.0f;
        QVERIFY(!checker.exceedsLimit(101, 100.0f, cfg));
    }
    void no_trigger_when_rate_below_limit() {
        RateOfChangeChecker checker;
        TagInf cfg;
        cfg.rateOfChangeLimit = 10.0f;
        checker.exceedsLimit(101, 100.0f, cfg);
        // Sleep briefly so dt > 0
        QTest::qWait(100);
        QVERIFY(!checker.exceedsLimit(101, 100.5f, cfg));
    }
    void trigger_when_rate_exceeds_limit() {
        RateOfChangeChecker checker;
        TagInf cfg;
        cfg.rateOfChangeLimit = 0.1f;
        checker.exceedsLimit(102, 100.0f, cfg);
        QTest::qWait(50);
        QVERIFY(checker.exceedsLimit(102, 120.0f, cfg));
    }
    void reset_clears_data() {
        RateOfChangeChecker checker;
        TagInf cfg;
        cfg.rateOfChangeLimit = 5.0f;
        checker.exceedsLimit(103, 100.0f, cfg);
        checker.reset(103);
        QVERIFY(!checker.exceedsLimit(103, 200.0f, cfg));
    }
};

QTEST_APPLESS_MAIN(TestRateOfChangeChecker)
#include "test_rate_of_change_checker.moc"
