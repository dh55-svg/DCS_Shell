#ifndef IALARMREPO_H
#define IALARMREPO_H
#include <QObject>
#include <QVector>
#include <QString>

// Forward declarations — full types in host/domain/alarm/AlarmEvent.h
struct AlarmEvent;
struct AlarmFilter;
struct AlarmChangeRecord;
struct AlarmKpiSnapshot;

class IAlarmRepo {
public:
    virtual ~IAlarmRepo() = default;
    virtual void insertEvent(const AlarmEvent& e) = 0;
    virtual void batchInsertEvents(const QVector<AlarmEvent>& events) = 0;
    virtual void updateAck(const QString& alarmId, const QString& user, qint64 ts) = 0;
    virtual void updateEvent(const QString& alarmId, const QString& field, const QString& value, qint64 ts) = 0;
    virtual QVector<AlarmEvent> queryActive() = 0;
    virtual QVector<AlarmEvent> queryEvents(const AlarmFilter& filter, int limit) = 0;
    virtual QVector<AlarmEvent> queryHistory(qint64 start, qint64 end, int limit) = 0;
    virtual void insertChangeRecord(const AlarmChangeRecord& r) = 0;
    virtual QVector<AlarmChangeRecord> queryChangeRecords(quint32 tagId, int limit) = 0;
    virtual QVector<AlarmChangeRecord> queryPendingApprovals() = 0;
    virtual void updateChangeApproval(int recordId, bool approved, const QString& approver, const QString& rejectReason) = 0;
    virtual void insertKpiSnapshot(const AlarmKpiSnapshot& s) = 0;
    virtual QVector<AlarmKpiSnapshot> queryKpiHistory(qint64 start, qint64 end, int limit) = 0;
    virtual void purgeOldRecords(int keepDays) = 0;
};

#define IAlarmRepo_iid "com.dcsshell.IAlarmRepo"
Q_DECLARE_INTERFACE(IAlarmRepo, IAlarmRepo_iid)
#endif
