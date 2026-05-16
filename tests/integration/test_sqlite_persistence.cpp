#include <QtTest>
#include <QTemporaryDir>
#include <QSqlDatabase>
#include <QSqlQuery>
#include "mocks/mock_history_repo.h"

class TestSqlitePersistence : public QObject {
    Q_OBJECT
private slots:
    void mock_history_repo_insert_and_query() {
        MockHistoryRepo repo;
        HistoryRecord r1{101, 42.5, 0, 1000};
        HistoryRecord r2{101, 43.0, 0, 2000};
        HistoryRecord r3{102, 99.0, 0, 1500};

        repo.batchInsert({r1, r2, r3});
        QCOMPARE(repo.insertCount, 1);

        auto results = repo.query(101, 0, 3000, 100);
        QCOMPARE(results.size(), 2);
    }
    void mock_alarm_repo_ack_tracking() {
        MockAlarmRepo repo;
        QCOMPARE(repo.ackCount, 0);
        repo.updateAck("ALM-001", "op", 1000);
        QCOMPARE(repo.ackCount, 1);
    }
};

QTEST_MAIN(TestSqlitePersistence)
#include "test_sqlite_persistence.moc"
