// =============================================================================
// MysqlPersistencePlugin.cpp — MySQL 持久化插件实现 (商业标准)
// =============================================================================
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
#include <QUuid>
#include <QDebug>

MysqlPersistencePlugin::MysqlPersistencePlugin(QObject* parent) : QObject(parent) {}

MysqlPersistencePlugin::~MysqlPersistencePlugin() {
    if (m_db.isOpen()) m_db.close();
    if (!m_connName.isEmpty())
        QSqlDatabase::removeDatabase(m_connName);
}

void MysqlPersistencePlugin::configure(const QVariantMap& config) {
    m_connName = "dcs_mysql_" + QUuid::createUuid().toString(QUuid::Id128);

    m_db = QSqlDatabase::addDatabase("QMYSQL", m_connName);
    m_db.setHostName(config.value("host", "127.0.0.1").toString());
    m_db.setPort(config.value("port", 3306).toInt());
    m_db.setDatabaseName(config.value("database", "dcs").toString());
    m_db.setUserName(config.value("user", "root").toString());
    m_db.setPassword(config.value("password", "").toString());
    m_db.setConnectOptions("MYSQL_OPT_RECONNECT=1;CLIENT_INTERACTIVE=1");

    if (!m_db.open()) {
        qCritical() << "[MySQL] connection failed:" << m_db.lastError().text();
        return;
    }
    initDb();
}

bool MysqlPersistencePlugin::execQuery(QSqlQuery& q) const {
    if (!q.exec()) {
        qWarning() << "[MySQL] SQL error:" << q.lastError().text()
                    << "| query:" << q.lastQuery();
        return false;
    }
    return true;
}

void MysqlPersistencePlugin::initDb() {
    QSqlQuery q(m_db);

    q.exec("CREATE TABLE IF NOT EXISTS alarm_events ("
           "alarm_id VARCHAR(64) PRIMARY KEY, tag_id INT NOT NULL, tag_name VARCHAR(128), "
           "description TEXT, limit_type TINYINT DEFAULT 0, priority TINYINT DEFAULT 2, "
           "classification TINYINT DEFAULT 0, state TINYINT DEFAULT 1, "
           "trigger_value FLOAT DEFAULT 0, threshold_value FLOAT DEFAULT 0, "
           "trigger_time BIGINT DEFAULT 0, acknowledged TINYINT DEFAULT 0, "
           "ack_time BIGINT DEFAULT 0, ack_user VARCHAR(64), active TINYINT DEFAULT 1, "
           "area VARCHAR(64), zone VARCHAR(64), "
           "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP, "
           "INDEX idx_alarm_state (state, active), INDEX idx_alarm_time (trigger_time)"
           ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4");

    q.exec("CREATE TABLE IF NOT EXISTS alarm_kpi_snapshots ("
           "id BIGINT AUTO_INCREMENT PRIMARY KEY, timestamp BIGINT NOT NULL, "
           "alarm_count_10min INT DEFAULT 0, avg_per_hour FLOAT DEFAULT 0, "
           "peak_count_10min INT DEFAULT 0, stale_count INT DEFAULT 0, "
           "total_active INT DEFAULT 0, shelved_count INT DEFAULT 0, "
           "suppressed_count INT DEFAULT 0, critical_count INT DEFAULT 0, "
           "major_count INT DEFAULT 0, minor_count INT DEFAULT 0, "
           "advisory_count INT DEFAULT 0, avg_ack_time_sec FLOAT DEFAULT 0, "
           "chattering_count INT DEFAULT 0, system_health_score FLOAT DEFAULT 100, "
           "health_grade VARCHAR(2), INDEX idx_kpi_ts (timestamp)"
           ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4");

    q.exec("CREATE TABLE IF NOT EXISTS history_records ("
           "id BIGINT AUTO_INCREMENT PRIMARY KEY, tag_id INT NOT NULL, "
           "value DOUBLE DEFAULT 0, quality INT DEFAULT 0, timestamp BIGINT NOT NULL, "
           "INDEX idx_history_tag_time (tag_id, timestamp)"
           ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4");

    q.exec("CREATE TABLE IF NOT EXISTS operation_logs ("
           "id BIGINT AUTO_INCREMENT PRIMARY KEY, user_name VARCHAR(64), "
           "action VARCHAR(128), target VARCHAR(256), detail TEXT, timestamp BIGINT, "
           "INDEX idx_op_time (timestamp)"
           ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4");
}

// ---------------------------------------------------------------------------
// IAlarmRepo — 已实现
// ---------------------------------------------------------------------------

void MysqlPersistencePlugin::insertEvent(const AlarmEvent& e) {
    QSqlQuery q(m_db);
    q.prepare("INSERT INTO alarm_events "
              "(alarm_id, tag_id, tag_name, description, limit_type, priority, "
              "classification, state, trigger_value, threshold_value, trigger_time, "
              "acknowledged, ack_time, ack_user, active, area, zone) "
              "VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?) "
              "ON DUPLICATE KEY UPDATE "
              "state=VALUES(state), acknowledged=VALUES(acknowledged), "
              "ack_time=VALUES(ack_time), ack_user=VALUES(ack_user), "
              "active=VALUES(active)");
    q.addBindValue(e.alarmId);
    q.addBindValue(e.tagId);
    q.addBindValue(e.tagName);
    q.addBindValue(e.description);
    q.addBindValue(static_cast<int>(e.limit));
    q.addBindValue(static_cast<int>(e.priority));
    q.addBindValue(static_cast<int>(e.classification));
    q.addBindValue(static_cast<int>(e.state));
    q.addBindValue(e.triggerValue);
    q.addBindValue(e.thresholdValue);
    q.addBindValue(e.triggerTime);
    q.addBindValue(e.acknowledged ? 1 : 0);
    q.addBindValue(e.acknowledgeTime);
    q.addBindValue(e.acknowledgeUser);
    q.addBindValue(e.active ? 1 : 0);
    q.addBindValue(e.area);
    q.addBindValue(e.zone);
    execQuery(q);
}

void MysqlPersistencePlugin::updateAck(const QString& alarmId, const QString& user, qint64 ts) {
    QSqlQuery q(m_db);
    q.prepare("UPDATE alarm_events SET acknowledged=1, ack_time=?, ack_user=? "
              "WHERE alarm_id=?");
    q.addBindValue(ts);
    q.addBindValue(user);
    q.addBindValue(alarmId);
    execQuery(q);
}

void MysqlPersistencePlugin::insertKpiSnapshot(const AlarmKpiSnapshot& s) {
    QSqlQuery q(m_db);
    q.prepare("INSERT INTO alarm_kpi_snapshots "
              "(timestamp, alarm_count_10min, avg_per_hour, peak_count_10min, "
              "stale_count, total_active, shelved_count, suppressed_count, "
              "critical_count, major_count, minor_count, advisory_count, "
              "avg_ack_time_sec, chattering_count, system_health_score, health_grade) "
              "VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)");
    q.addBindValue(s.timestamp);
    q.addBindValue(s.alarmCount10min);
    q.addBindValue(s.avgPerHour);
    q.addBindValue(s.peakCount10min);
    q.addBindValue(s.staleCount);
    q.addBindValue(s.totalActive);
    q.addBindValue(s.shelvedCount);
    q.addBindValue(s.suppressedCount);
    q.addBindValue(s.criticalCount);
    q.addBindValue(s.majorCount);
    q.addBindValue(s.minorCount);
    q.addBindValue(s.advisoryCount);
    q.addBindValue(s.avgAckTimeSec);
    q.addBindValue(s.chatteringCount);
    q.addBindValue(s.systemHealthScore);
    q.addBindValue(s.healthGrade);
    execQuery(q);
}

// ---------------------------------------------------------------------------
// IHistoryRepo — 已实现
// ---------------------------------------------------------------------------

void MysqlPersistencePlugin::batchInsert(const QVector<HistoryRecord>& records) {
    if (records.isEmpty()) return;
    if (!m_db.isOpen()) {
        qWarning() << "[MySQL] batchInsert: database not open";
        return;
    }
    if (!m_db.transaction()) {
        qWarning() << "[MySQL] batchInsert: transaction failed:" << m_db.lastError().text();
        return;
    }
    QSqlQuery q(m_db);
    q.prepare("INSERT INTO history_records (tag_id, value, quality, timestamp) "
              "VALUES (?,?,?,?)");
    for (const auto& r : records) {
        q.addBindValue(r.tagId);
        q.addBindValue(r.value);
        q.addBindValue(r.quality);
        q.addBindValue(r.timestamp);
        if (!q.exec()) {
            qWarning() << "[MySQL] batchInsert item failed:" << q.lastError().text();
            m_db.rollback();
            return;
        }
    }
    if (!m_db.commit()) {
        qWarning() << "[MySQL] batchInsert commit failed:" << m_db.lastError().text();
        m_db.rollback();
    }
}

QVector<HistoryRecord> MysqlPersistencePlugin::query(quint32 tagId, qint64 startTime,
                                                      qint64 endTime, int maxPoints) {
    QVector<HistoryRecord> result;
    QSqlQuery q(m_db);
    q.prepare("SELECT tag_id, value, quality, timestamp FROM history_records "
              "WHERE tag_id=? AND timestamp>=? AND timestamp<=? "
              "ORDER BY timestamp LIMIT ?");
    q.addBindValue(tagId);
    q.addBindValue(startTime);
    q.addBindValue(endTime);
    q.addBindValue(maxPoints);
    if (!execQuery(q)) return result;
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

// ---------------------------------------------------------------------------
// IAlarmRepo — stub (未被业务调用, 打 warning 提示)
// ---------------------------------------------------------------------------

void MysqlPersistencePlugin::batchInsertEvents(const QVector<AlarmEvent>&) {
    qWarning() << "[MySQL] batchInsertEvents not implemented";
}

void MysqlPersistencePlugin::updateEvent(const QString&, const QString&, const QString&, qint64) {
    qWarning() << "[MySQL] updateEvent not implemented";
}

QVector<AlarmEvent> MysqlPersistencePlugin::queryActive() {
    qWarning() << "[MySQL] queryActive not implemented";
    return {};
}

QVector<AlarmEvent> MysqlPersistencePlugin::queryEvents(const AlarmFilter&, int) {
    qWarning() << "[MySQL] queryEvents not implemented";
    return {};
}

QVector<AlarmEvent> MysqlPersistencePlugin::queryHistory(qint64, qint64, int) {
    qWarning() << "[MySQL] queryHistory not implemented";
    return {};
}

void MysqlPersistencePlugin::insertChangeRecord(const AlarmChangeRecord&) {
    qWarning() << "[MySQL] insertChangeRecord not implemented";
}

QVector<AlarmChangeRecord> MysqlPersistencePlugin::queryChangeRecords(quint32, int) {
    qWarning() << "[MySQL] queryChangeRecords not implemented";
    return {};
}

QVector<AlarmChangeRecord> MysqlPersistencePlugin::queryPendingApprovals() {
    qWarning() << "[MySQL] queryPendingApprovals not implemented";
    return {};
}

void MysqlPersistencePlugin::updateChangeApproval(int, bool, const QString&, const QString&) {
    qWarning() << "[MySQL] updateChangeApproval not implemented";
}

QVector<AlarmKpiSnapshot> MysqlPersistencePlugin::queryKpiHistory(qint64, qint64, int) {
    qWarning() << "[MySQL] queryKpiHistory not implemented";
    return {};
}

void MysqlPersistencePlugin::purgeOldRecords(int) {
    qWarning() << "[MySQL] purgeOldRecords not implemented";
}

// ---------------------------------------------------------------------------
// ITagRepo — stub (零调用, 返回 false 防止误报成功)
// ---------------------------------------------------------------------------

bool MysqlPersistencePlugin::insert(const TagInf&) {
    qWarning() << "[MySQL] TagRepo::insert not implemented";
    return false;
}

bool MysqlPersistencePlugin::update(const TagInf&) {
    qWarning() << "[MySQL] TagRepo::update not implemented";
    return false;
}

bool MysqlPersistencePlugin::remove(quint32) {
    qWarning() << "[MySQL] TagRepo::remove not implemented";
    return false;
}

TagInf MysqlPersistencePlugin::findById(quint32) const {
    qWarning() << "[MySQL] TagRepo::findById not implemented";
    return TagInf{};
}

QVector<TagInf> MysqlPersistencePlugin::findAll() const {
    qWarning() << "[MySQL] TagRepo::findAll not implemented";
    return {};
}

bool MysqlPersistencePlugin::loadFromJson(const QString&) {
    qWarning() << "[MySQL] TagRepo::loadFromJson not implemented";
    return false;
}

bool MysqlPersistencePlugin::saveToJson(const QString&) const {
    qWarning() << "[MySQL] TagRepo::saveToJson not implemented";
    return false;
}

// ---------------------------------------------------------------------------
// IOperationRepo — stub (零调用, 表已建好但方法未实现)
// ---------------------------------------------------------------------------

void MysqlPersistencePlugin::log(const QString&, const QString&, const QString&, const QString&) {
    qWarning() << "[MySQL] OperationRepo::log not implemented";
}

QVector<QJsonObject> MysqlPersistencePlugin::query(qint64, qint64, int) {
    qWarning() << "[MySQL] OperationRepo::query not implemented";
    return {};
}

// ---------------------------------------------------------------------------
// 结果集转换辅助 (供未来 query 方法使用)
// ---------------------------------------------------------------------------

AlarmEvent MysqlPersistencePlugin::toAlarmEvent(QSqlQuery&) const {
    return AlarmEvent{};
}

AlarmChangeRecord MysqlPersistencePlugin::toChangeRecord(QSqlQuery&) const {
    return AlarmChangeRecord{};
}

AlarmKpiSnapshot MysqlPersistencePlugin::toKpiSnapshot(QSqlQuery&) const {
    return AlarmKpiSnapshot{};
}

TagInf MysqlPersistencePlugin::toTagInf(QSqlQuery&) const {
    return TagInf{};
}
