#ifndef MYSQLPERSISTENCEPLUGIN_H
#define MYSQLPERSISTENCEPLUGIN_H
#include <QObject>
#include <QSqlDatabase>
#include "plugin_interface/IAlarmRepo.h"
#include "plugin_interface/IHistoryRepo.h"
#include "plugin_interface/ITagRepo.h"
#include "plugin_interface/IOperationRepo.h"
#include "plugin_interface/IConfigurable.h"

class MysqlPersistencePlugin : public QObject,
    public IAlarmRepo,
    public IHistoryRepo,
    public ITagRepo,
    public IOperationRepo,
    public IConfigurable {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "com.dcsshell.MysqlPersistence" FILE "MysqlPersistencePlugin.json")

public:
    explicit MysqlPersistencePlugin(QObject* parent = nullptr);
    ~MysqlPersistencePlugin() override;

    // IConfigurable
    void configure(const QVariantMap& config) override;

    // IAlarmRepo
    void insertEvent(const AlarmEvent& e) override;
    void batchInsertEvents(const QVector<AlarmEvent>& events) override;
    void updateAck(const QString& alarmId, const QString& user, qint64 ts) override;
    void updateEvent(const QString& alarmId, const QString& field, const QString& value, qint64 ts) override;
    QVector<AlarmEvent> queryActive() override;
    QVector<AlarmEvent> queryEvents(const AlarmFilter& filter, int limit) override;
    QVector<AlarmEvent> queryHistory(qint64 start, qint64 end, int limit) override;
    void insertChangeRecord(const AlarmChangeRecord& r) override;
    QVector<AlarmChangeRecord> queryChangeRecords(quint32 tagId, int limit) override;
    QVector<AlarmChangeRecord> queryPendingApprovals() override;
    void updateChangeApproval(int recordId, bool approved, const QString& approver, const QString& rejectReason) override;
    void insertKpiSnapshot(const AlarmKpiSnapshot& s) override;
    QVector<AlarmKpiSnapshot> queryKpiHistory(qint64 start, qint64 end, int limit) override;
    void purgeOldRecords(int keepDays) override;

    // IHistoryRepo
    void batchInsert(const QVector<HistoryRecord>& records) override;
    QVector<HistoryRecord> query(quint32 tagId, qint64 startTime, qint64 endTime, int maxPoints) override;

    // ITagRepo
    bool insert(const TagInf& tag) override;
    bool update(const TagInf& tag) override;
    bool remove(quint32 tagId) override;
    TagInf findById(quint32 tagId) const override;
    QVector<TagInf> findAll() const override;
    bool loadFromJson(const QString& path) override;
    bool saveToJson(const QString& path) const override;

    // IOperationRepo
    void log(const QString& user, const QString& action, const QString& target, const QString& detail) override;
    QVector<QJsonObject> query(qint64 start, qint64 end, int limit) override;

private:
    void initDb();
    QSqlDatabase m_db;
    bool m_configured = false;
};
#endif
