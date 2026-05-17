#include "MysqlPersistencePlugin.h"
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

MysqlPersistencePlugin::MysqlPersistencePlugin(QObject* parent) : QObject(parent) {}

MysqlPersistencePlugin::~MysqlPersistencePlugin() {
    if (m_db.isOpen()) m_db.close();
}

void MysqlPersistencePlugin::configure(const QVariantMap& config) {
    QString host = config.value("host", "127.0.0.1").toString();
    int port = config.value("port", 3306).toInt();
    QString database = config.value("database", "dcs").toString();
    QString user = config.value("user", "root").toString();
    QString password = config.value("password", "").toString();

    m_db = QSqlDatabase::addDatabase("QMYSQL", "persistence_mysql");
    m_db.setHostName(host);
    m_db.setPort(port);
    m_db.setDatabaseName(database);
    m_db.setUserName(user);
    m_db.setPassword(password);

    if (!m_db.open()) {
        qWarning() << "MysqlPersistencePlugin: cannot connect:" << m_db.lastError().text();
        return;
    }
    m_configured = true;
    initDb();
}

void MysqlPersistencePlugin::initDb() {
    QSqlQuery q(m_db);
    q.exec("CREATE TABLE IF NOT EXISTS alarm_events ("
           "alarm_id VARCHAR(64) PRIMARY KEY, tag_id INT, tag_name VARCHAR(128), description TEXT, "
           "limit_type INT, priority INT, classification INT, state INT, "
           "trigger_value FLOAT, threshold_value FLOAT, trigger_time BIGINT, ack_time BIGINT, "
           "ack_user VARCHAR(64), unit VARCHAR(32), area VARCHAR(64), zone VARCHAR(64)"
           ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4");
    q.exec("CREATE TABLE IF NOT EXISTS history_records ("
           "tag_id INT, value DOUBLE, quality INT, timestamp BIGINT, "
           "INDEX idx_history_tag_time (tag_id, timestamp)"
           ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4");
    q.exec("CREATE TABLE IF NOT EXISTS operation_logs ("
           "id INT AUTO_INCREMENT PRIMARY KEY, user VARCHAR(64), action VARCHAR(128), "
           "target VARCHAR(256), detail TEXT, timestamp BIGINT"
           ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4");
}

// IAlarmRepo stubs
void MysqlPersistencePlugin::insertEvent(const AlarmEvent&) {}
void MysqlPersistencePlugin::batchInsertEvents(const QVector<AlarmEvent>&) {}
void MysqlPersistencePlugin::updateAck(const QString&, const QString&, qint64) {}
void MysqlPersistencePlugin::updateEvent(const QString&, const QString&, const QString&, qint64) {}
QVector<AlarmEvent> MysqlPersistencePlugin::queryActive() { return {}; }
QVector<AlarmEvent> MysqlPersistencePlugin::queryEvents(const AlarmFilter&, int) { return {}; }
QVector<AlarmEvent> MysqlPersistencePlugin::queryHistory(qint64, qint64, int) { return {}; }
void MysqlPersistencePlugin::insertChangeRecord(const AlarmChangeRecord&) {}
QVector<AlarmChangeRecord> MysqlPersistencePlugin::queryChangeRecords(quint32, int) { return {}; }
QVector<AlarmChangeRecord> MysqlPersistencePlugin::queryPendingApprovals() { return {}; }
void MysqlPersistencePlugin::updateChangeApproval(int, bool, const QString&, const QString&) {}
void MysqlPersistencePlugin::insertKpiSnapshot(const AlarmKpiSnapshot&) {}
QVector<AlarmKpiSnapshot> MysqlPersistencePlugin::queryKpiHistory(qint64, qint64, int) { return {}; }
void MysqlPersistencePlugin::purgeOldRecords(int) {}

// IHistoryRepo
void MysqlPersistencePlugin::batchInsert(const QVector<HistoryRecord>& records) {
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

QVector<HistoryRecord> MysqlPersistencePlugin::query(quint32 tagId, qint64 start, qint64 end, int maxPoints) {
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
bool MysqlPersistencePlugin::insert(const TagInf&) { return true; }
bool MysqlPersistencePlugin::update(const TagInf&) { return true; }
bool MysqlPersistencePlugin::remove(quint32) { return true; }
TagInf MysqlPersistencePlugin::findById(quint32) const { return TagInf{}; }
QVector<TagInf> MysqlPersistencePlugin::findAll() const { return {}; }
bool MysqlPersistencePlugin::loadFromJson(const QString&) { return true; }
bool MysqlPersistencePlugin::saveToJson(const QString&) const { return true; }

// IOperationRepo
void MysqlPersistencePlugin::log(const QString& user, const QString& action,
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

QVector<QJsonObject> MysqlPersistencePlugin::query(qint64 start, qint64 end, int limit) {
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
