#include <QtTest>
#include <QDateTime>
#include "domain/alarm/ShelveManager.h"

class TestShelveManager : public QObject {
    Q_OBJECT
private slots:
    void shelve_adds_deadline() {
        ShelveManager sm;
        sm.shelve(101, 300, "test", "op", false);
        QCOMPARE(sm.count(), 1);
    }
    void unshelve_removes_deadline() {
        ShelveManager sm;
        sm.shelve(101, 300, "test");
        sm.unshelve(101);
        QCOMPARE(sm.count(), 0);
    }
    void expired_after_duration() {
        ShelveManager sm;
        sm.shelve(101, -1, "test"); // negative = already expired
        auto result = sm.checkExpired();
        QVERIFY(!result.normalExpired.isEmpty());
    }
    void auto_shelve_tracks_count() {
        ShelveManager sm;
        for (int i = 0; i < 5; ++i) {
            sm.shelve(101, 10, "auto", "", true);
            sm.unshelve(101);
        }
        QVERIFY(sm.shelveCount(101) >= 3);
    }
    void history_recorded() {
        ShelveManager sm;
        sm.shelve(101, 300, "reason", "user");
        auto hist = sm.history();
        QCOMPARE(hist.size(), 1);
        QCOMPARE(hist[0].tagId, 101u);
        QCOMPARE(hist[0].reason, "reason");
    }
};

QTEST_MAIN(TestShelveManager)
#include "test_shelve_manager.moc"
