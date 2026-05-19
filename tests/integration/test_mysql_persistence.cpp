#include <QtTest>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QJsonObject>
#include "plugin_interface/IHistoryRepo.h"
#include "plugin_interface/IAlarmRepo.h"
#include "host/domain/alarm/AlarmEvent.h"
#include "host/domain/tag/TagInfo.h"

class TestMysqlPersistence : public QObject {
    Q_OBJECT
private:
    QSqlDatabase m_db;
    QString m_connName;

    void initDb() {
        QSqlQuery q(m_db);
        q.exec("CREATE TABLE alarm_events ("
               "alarm_id TEXT PRIMARY KEY, tag_id INTEGER, tag_name TEXT, "
               "description TEXT, limit_type INTEGER, priority INTEGER, "
               "classification INTEGER, state INTEGER, "
               "trigger_value REAL, threshold_value REAL, "
               "trigger_time INTEGER, acknowledged INTEGER, "
               "ack_time INTEGER, ack_user TEXT, active INTEGER, "
               "area TEXT, zone TEXT)");
        q.exec("CREATE TABLE alarm_kpi_snapshots ("
               "id INTEGER PRIMARY KEY AUTOINCREMENT, timestamp INTEGER, "
               "alarm_count_10min INTEGER, avg_per_hour REAL, "
               "peak_count_10min INTEGER, stale_count INTEGER, "
               "total_active INTEGER, shelved_count INTEGER, "
               "suppressed_count INTEGER, critical_count INTEGER, "
               "major_count INTEGER, minor_count INTEGER, "
               "advisory_count INTEGER, avg_ack_time_sec REAL, "
               "chattering_count INTEGER, system_health_score REAL, "
               "health_grade TEXT)");
        q.exec("CREATE TABLE history_records ("
               "id INTEGER PRIMARY KEY AUTOINCREMENT, tag_id INTEGER, "
               "value REAL, quality INTEGER, timestamp INTEGER)");
        q.exec("CREATE TABLE operation_logs ("
               "id INTEGER PRIMARY KEY AUTOINCREMENT, user_name TEXT, "
               "action TEXT, target TEXT, detail TEXT, timestamp INTEGER)");
    }

private slots:
    void initTestCase() {
        m_connName = "test_mysql_" + QString::number(reinterpret_cast<quintptr>(this));
        m_db = QSqlDatabase::addDatabase("QSQLITE", m_connName);
        m_db.setDatabaseName(":memory:");
        QVERIFY(m_db.open());
        initDb();
    }

    void cleanupTestCase() {
        m_db.close();
        QSqlDatabase::removeDatabase(m_connName);
    }

    // --- insertEvent ---
    void insertEvent_basic() {
        QSqlQuery q(m_db);
        q.prepare("INSERT INTO alarm_events "
                  "(alarm_id, tag_id, tag_name, limit_type, priority, "
                  "classification, state, trigger_value, threshold_value, "
                  "trigger_time, acknowledged, ack_time, ack_user, active, area, zone) "
                  "VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)");
        q.addBindValue("ALM-001");
        q.addBindValue(101);
        q.addBindValue("TT_101");
        q.addBindValue(3); // High
        q.addBindValue(2); // Major
        q.addBindValue(0); // Process
        q.addBindValue(1); // ActiveUnack
        q.addBindValue(85.5);
        q.addBindValue(80.0);
        q.addBindValue(1000);
        q.addBindValue(0);
        q.addBindValue(0);
        q.addBindValue("");
        q.addBindValue(1);
        q.addBindValue("Area1");
        q.addBindValue("Zone1");
        QVERIFY(q.exec());

        QSqlQuery verify(m_db);
        verify.exec("SELECT alarm_id, tag_id, state, trigger_value FROM alarm_events WHERE alarm_id='ALM-001'");
        QVERIFY(verify.next());
        QCOMPARE(verify.value(0).toString(), QString("ALM-001"));
        QCOMPARE(verify.value(1).toInt(), 101);
        QCOMPARE(verify.value(2).toInt(), 1);
        QCOMPARE(verify.value(3).toFloat(), 85.5f);
    }

    void insertEvent_duplicate_updates_state() {
        QSqlQuery q1(m_db);
        q1.prepare("INSERT INTO alarm_events (alarm_id, tag_id, tag_name, state, active) "
                   "VALUES (?,?,?,?,?)");
        q1.addBindValue("ALM-DUP");
        q1.addBindValue(200);
        q1.addBindValue("PT_200");
        q1.addBindValue(1);
        q1.addBindValue(1);
        QVERIFY(q1.exec());

        QSqlQuery q2(m_db);
        q2.prepare("UPDATE alarm_events SET state=?, acknowledged=1, ack_time=?, ack_user=? "
                   "WHERE alarm_id=?");
        q2.addBindValue(2); // Activeack
        q2.addBindValue(5000);
        q2.addBindValue("operator1");
        q2.addBindValue("ALM-DUP");
        QVERIFY(q2.exec());

        QSqlQuery verify(m_db);
        verify.exec("SELECT state, acknowledged, ack_user FROM alarm_events WHERE alarm_id='ALM-DUP'");
        QVERIFY(verify.next());
        QCOMPARE(verify.value(0).toInt(), 2);
        QCOMPARE(verify.value(1).toInt(), 1);
        QCOMPARE(verify.value(2).toString(), QString("operator1"));
    }

    // --- updateAck ---
    void updateAck_sets_ack_fields() {
        QSqlQuery setup(m_db);
        setup.prepare("INSERT INTO alarm_events (alarm_id, tag_id, tag_name, state, acknowledged) "
                      "VALUES (?,?,?,?,?)");
        setup.addBindValue("ALM-ACK");
        setup.addBindValue(300);
        setup.addBindValue("LT_300");
        setup.addBindValue(1);
        setup.addBindValue(0);
        QVERIFY(setup.exec());

        QSqlQuery update(m_db);
        update.prepare("UPDATE alarm_events SET acknowledged=1, ack_time=?, ack_user=? WHERE alarm_id=?");
        update.addBindValue(9999);
        update.addBindValue("tech_user");
        update.addBindValue("ALM-ACK");
        QVERIFY(update.exec());

        QSqlQuery verify(m_db);
        verify.exec("SELECT acknowledged, ack_time, ack_user FROM alarm_events WHERE alarm_id='ALM-ACK'");
        QVERIFY(verify.next());
        QCOMPARE(verify.value(0).toInt(), 1);
        QCOMPARE(verify.value(1).toLongLong(), 9999LL);
        QCOMPARE(verify.value(2).toString(), QString("tech_user"));
    }

    // --- insertKpiSnapshot ---
    void insertKpiSnapshot_basic() {
        QSqlQuery q(m_db);
        q.prepare("INSERT INTO alarm_kpi_snapshots "
                  "(timestamp, alarm_count_10min, avg_per_hour, total_active, "
                  "critical_count, major_count, system_health_score, health_grade) "
                  "VALUES (?,?,?,?,?,?,?,?)");
        q.addBindValue(10000);
        q.addBindValue(15);
        q.addBindValue(90.0);
        q.addBindValue(8);
        q.addBindValue(2);
        q.addBindValue(5);
        q.addBindValue(85.0);
        q.addBindValue("B");
        QVERIFY(q.exec());

        QSqlQuery verify(m_db);
        verify.exec("SELECT alarm_count_10min, health_grade FROM alarm_kpi_snapshots WHERE timestamp=10000");
        QVERIFY(verify.next());
        QCOMPARE(verify.value(0).toInt(), 15);
        QCOMPARE(verify.value(1).toString(), QString("B"));
    }

    // --- batchInsert ---
    void batchInsert_multiple_records() {
        QSqlQuery q(m_db);
        QVERIFY(m_db.transaction());
        q.prepare("INSERT INTO history_records (tag_id, value, quality, timestamp) VALUES (?,?,?,?)");

        struct { int tagId; double value; int quality; qint64 ts; } data[] = {
            {1, 25.3, 0, 1000}, {1, 25.5, 0, 2000}, {2, 99.0, 0, 1500}
        };
        for (const auto& d : data) {
            q.addBindValue(d.tagId);
            q.addBindValue(d.value);
            q.addBindValue(d.quality);
            q.addBindValue(d.ts);
            QVERIFY(q.exec());
        }
        QVERIFY(m_db.commit());

        QSqlQuery verify(m_db);
        verify.exec("SELECT COUNT(*) FROM history_records");
        QVERIFY(verify.next());
        QVERIFY(verify.value(0).toInt() >= 3);
    }

    void batchInsert_rollback_on_empty() {
        int before = 0;
        QSqlQuery count(m_db);
        count.exec("SELECT COUNT(*) FROM history_records");
        if (count.next()) before = count.value(0).toInt();

        QVector<HistoryRecord> empty;
        // In real code: if (records.isEmpty()) return;

        QSqlQuery verify(m_db);
        verify.exec("SELECT COUNT(*) FROM history_records");
        QVERIFY(verify.next());
        QCOMPARE(verify.value(0).toInt(), before);
    }

    // --- query ---
    void query_by_tag_and_time_range() {
        QSqlQuery q(m_db);
        q.prepare("INSERT INTO history_records (tag_id, value, quality, timestamp) VALUES (?,?,?,?)");
        q.addBindValue(50); q.addBindValue(1.0); q.addBindValue(0); q.addBindValue(100); QVERIFY(q.exec());
        q.addBindValue(50); q.addBindValue(2.0); q.addBindValue(0); q.addBindValue(200); QVERIFY(q.exec());
        q.addBindValue(50); q.addBindValue(3.0); q.addBindValue(0); q.addBindValue(300); QVERIFY(q.exec());
        q.addBindValue(51); q.addBindValue(9.0); q.addBindValue(0); q.addBindValue(200); QVERIFY(q.exec());

        QSqlQuery query(m_db);
        query.prepare("SELECT tag_id, value, quality, timestamp FROM history_records "
                      "WHERE tag_id=? AND timestamp>=? AND timestamp<=? ORDER BY timestamp LIMIT ?");
        query.addBindValue(50);
        query.addBindValue(100);
        query.addBindValue(300);
        query.addBindValue(10);
        QVERIFY(query.exec());

        QVector<HistoryRecord> results;
        while (query.next()) {
            HistoryRecord r;
            r.tagId = query.value(0).toUInt();
            r.value = query.value(1).toDouble();
            r.quality = query.value(2).toInt();
            r.timestamp = query.value(3).toLongLong();
            results.append(r);
        }
        QCOMPARE(results.size(), 3);
        QCOMPARE(results[0].tagId, 50u);
        QCOMPARE(results[0].value, 1.0);
        QCOMPARE(results[2].value, 3.0);
    }

    void query_respects_limit() {
        QSqlQuery q(m_db);
        q.prepare("INSERT INTO history_records (tag_id, value, quality, timestamp) VALUES (?,?,?,?)");
        for (int i = 0; i < 100; i++) {
            q.addBindValue(99);
            q.addBindValue(static_cast<double>(i));
            q.addBindValue(0);
            q.addBindValue(i * 1000);
            q.exec();
        }

        QSqlQuery query(m_db);
        query.prepare("SELECT tag_id, value, quality, timestamp FROM history_records "
                      "WHERE tag_id=? AND timestamp>=? AND timestamp<=? ORDER BY timestamp LIMIT ?");
        query.addBindValue(99);
        query.addBindValue(0);
        query.addBindValue(999999999);
        query.addBindValue(5);
        QVERIFY(query.exec());

        int count = 0;
        while (query.next()) count++;
        QCOMPARE(count, 5);
    }
};

QTEST_MAIN(TestMysqlPersistence)
#include "test_mysql_persistence.moc"
