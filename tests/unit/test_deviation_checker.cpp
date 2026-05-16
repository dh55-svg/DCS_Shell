#include <QtTest>
#include "domain/tag/DeviationChecker.h"

class TestDeviationChecker : public QObject {
    Q_OBJECT
private slots:
    void no_deviation_when_within_limit() {
        QVERIFY(!DeviationChecker::exceedsDeviation(80.0f, 85.0f, 10.0f));
    }
    void deviation_exceeded() {
        QVERIFY(DeviationChecker::exceedsDeviation(80.0f, 95.0f, 10.0f));
    }
    void exact_at_limit_not_exceeded() {
        QVERIFY(!DeviationChecker::exceedsDeviation(100.0f, 105.0f, 5.0f));
    }
    void negative_deviation() {
        QVERIFY(DeviationChecker::exceedsDeviation(100.0f, 80.0f, 10.0f));
    }
};

QTEST_APPLESS_MAIN(TestDeviationChecker)
#include "test_deviation_checker.moc"
