#include "SqlitePersistencePlugin.h"
#include "host/domain/alarm/AlarmEvent.h"
#include "host/domain/tag/TagInfo.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QFile>
#include <QDateTime>
#include <QDebug>

SqlitePersistencePlugin::SqlitePersistencePlugin(QObject* parent) : QObject(parent) {
    initDb();
}

SqlitePersistencePlugin::~SqlitePersistencePlugin() {
    if (m_db.isOpen()) m_db.close();
}

void SqlitePersistencePlugin::initDb() {
    m_db = QSqlDatabase::addDatabase("QSQLITE", "persistence_plugin");
    m_db.setDatabaseName("data/dcs.db");
    if (!m_db.open()) {
        qWarning() << "SqlitePersistencePlugin: cannot open database:" << m_db.lastError().text();
        return;
    }
    QSqlQuery q(m_db);
    q.exec("CREATE TABLE IF NOT EXISTS alarm_events ("
           "alarm_id TEXT PRIMARY KEY, tag_id INTEGER, tag_name TEXT, description TEXT, "
           "limit_type INTEGER, priority INTEGER, classification INTEGER, state INTEGER, "
           "trigger_value REAL, threshold_value REAL, trigger_time INTEGER, ack_time INTEGER, "
           "ack_user TEXT, unit TEXT, area TEXT, zone TEXT)");
    q.exec("CREATE TABLE IF NOT EXISTS history_records ("
           "tag_id INTEGER, value REAL, quality INTEGER, timestamp INTEGER)");
    q.exec("CREATE INDEX IF NOT EXISTS idx_history_tag_time ON history_records(tag_id, timestamp)");
    q.exec("CREATE TABLE IF NOT EXISTS operation_logs ("
           "id INTEGER PRIMARY KEY AUTOINCREMENT, user TEXT, action TEXT, "
           "target TEXT, detail TEXT, timestamp INTEGER)");
}

// IAlarmRepo stubs
void SqlitePersistencePlugin::insertEvent(const AlarmEvent&) {}
void SqlitePersistencePlugin::batchInsertEvents(const QVector<AlarmEvent>&) {}
void SqlitePersistencePlugin::updateAck(const QString&, const QString&, qint64) {}
void SqlitePersistencePlugin::updateEvent(const QString&, const QString&, const QString&, qint64) {}
QVector<AlarmEvent> SqlitePersistencePlugin::queryActive() { return {}; }
QVector<AlarmEvent> SqlitePersistencePlugin::queryEvents(const AlarmFilter&, int) { return {}; }
QVector<AlarmEvent> SqlitePersistencePlugin::queryHistory(qint64, qint64, int) { return {}; }
void SqlitePersistencePlugin::insertChangeRecord(const AlarmChangeRecord&) {}
QVector<AlarmChangeRecord> SqlitePersistencePlugin::queryChangeRecords(quint32, int) { return {}; }
QVector<AlarmChangeRecord> SqlitePersistencePlugin::queryPendingApprovals() { return {}; }
void SqlitePersistencePlugin::updateChangeApproval(int, bool, const QString&, const QString&) {}
void SqlitePersistencePlugin::insertKpiSnapshot(const AlarmKpiSnapshot&) {}
QVector<AlarmKpiSnapshot> SqlitePersistencePlugin::queryKpiHistory(qint64, qint64, int) { return {}; }
void SqlitePersistencePlugin::purgeOldRecords(int) {}

// IHistoryRepo
void SqlitePersistencePlugin::batchInsert(const QVector<HistoryRecord>& records) {
    if (!m_db.isOpen()) return;
    QSqlQuery q(m_db);
    q.prepare("INSERT INTO history_records(tag_id, value, quality, timestamp) VALUES(?,?,?,?)");
    for (const auto& r : records) {
        q.addBindValue(r.tagId);
        q.addBindValue(r.value);
        q.addBindValue(r.quality);
        q.addBindValue(r.timestamp);
        q.exec();
    }
}

QVector<HistoryRecord> SqlitePersistencePlugin::query(quint32 tagId, qint64 start, qint64 end, int maxPoints) {
    QVector<HistoryRecord> result;
    if (!m_db.isOpen()) return result;
    QSqlQuery q(m_db);
    q.prepare("SELECT tag_id, value, quality, timestamp FROM history_records "
              "WHERE tag_id=? AND timestamp>=? AND timestamp<=? ORDER BY timestamp LIMIT ?");
    q.addBindValue(tagId);
    q.addBindValue(start);
    q.addBindValue(end);
    q.addBindValue(maxPoints);
    q.exec();
    while (q.next()) {
        HistoryRecord r;
        r.tagId = q.value(0).toUInt();
        r.value = q.value(1).toDouble();
        r.quality = q.value(2).toInt();
        r.timestamp = q.value(3).toLongLong();
        result.append(r);
    }
    return result;
}

// ITagRepo stubs
bool SqlitePersistencePlugin::insert(const TagInf&) { return true; }
bool SqlitePersistencePlugin::update(const TagInf&) { return true; }
bool SqlitePersistencePlugin::remove(quint32) { return true; }
TagInf SqlitePersistencePlugin::findById(quint32) const { return TagInf{}; }
QVector<TagInf> SqlitePersistencePlugin::findAll() const { return {}; }
bool SqlitePersistencePlugin::loadFromJson(const QString&) { return true; }
bool SqlitePersistencePlugin::saveToJson(const QString&) const { return true; }

// IOperationRepo
void SqlitePersistencePlugin::log(const QString& user, const QString& action,
                                   const QString& target, const QString& detail) {
    if (!m_db.isOpen()) return;
    QSqlQuery q(m_db);
    q.prepare("INSERT INTO operation_logs(user, action, target, detail, timestamp) VALUES(?,?,?,?,?)");
    q.addBindValue(user);
    q.addBindValue(action);
    q.addBindValue(target);
    q.addBindValue(detail);
    q.addBindValue(QDateTime::currentMSecsSinceEpoch());
    q.exec();
}

QVector<QJsonObject> SqlitePersistencePlugin::query(qint64 start, qint64 end, int limit) {
    QVector<QJsonObject> result;
    if (!m_db.isOpen()) return result;
    QSqlQuery q(m_db);
    q.prepare("SELECT user, action, target, detail, timestamp FROM operation_logs "
              "WHERE timestamp>=? AND timestamp<=? ORDER BY timestamp DESC LIMIT ?");
    q.addBindValue(start);
    q.addBindValue(end);
    q.addBindValue(limit);
    q.exec();
    while (q.next()) {
        QJsonObject obj;
        obj["user"] = q.value(0).toString();
        obj["action"] = q.value(1).toString();
        obj["target"] = q.value(2).toString();
        obj["detail"] = q.value(3).toString();
        obj["timestamp"] = q.value(4).toLongLong();
        result.append(obj);
    }
    return result;
}
