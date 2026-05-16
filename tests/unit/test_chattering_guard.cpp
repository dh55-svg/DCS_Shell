#include <QtTest>
#include "domain/alarm/ChatteringGuard.h"

class TestChatteringGuard : public QObject {
    Q_OBJECT
private slots:
    void no_chattering_with_few_repeats() {
        ChatteringGuard cg;
        for (int i = 0; i < 2; ++i)
            QVERIFY(!cg.check(101, 3));
    }
    void chattering_detected_at_threshold() {
        ChatteringGuard cg;
        for (int i = 0; i < 2; ++i) cg.check(102, 3);
        QVERIFY(cg.check(102, 3));
    }
    void reset_clears_state() {
        ChatteringGuard cg;
        cg.check(103, 3);
        cg.check(103, 3);
        cg.reset(103);
        QVERIFY(!cg.check(103, 3));
    }
};

QTEST_APPLESS_MAIN(TestChatteringGuard)
#include "test_chattering_guard.moc"
