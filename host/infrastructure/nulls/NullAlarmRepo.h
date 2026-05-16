#ifndef NULLALARMREPO_H
#define NULLALARMREPO_H
#include "plugin_interface/IAlarmRepo.h"
#include "../logging/ILogger.h"

class NullAlarmRepo : public IAlarmRepo {
public:
    explicit NullAlarmRepo(ILogger* logger = nullptr) : m_logger(logger) {}
    void insertEvent(const AlarmEvent&) override {
        if (m_logger) m_logger->warn("NullAlarmRepo: alarm not persisted (no persistence plugin)");
    }
    void batchInsertEvents(const QVector<AlarmEvent>&) override {}
    void updateAck(const QString&, const QString&, qint64) override {}
    void updateEvent(const QString&, const QString&, const QString&, qint64) override {}
    QVector<AlarmEvent> queryActive() override { return {}; }
    QVector<AlarmEvent> queryEvents(const AlarmFilter&, int) override { return {}; }
    QVector<AlarmEvent> queryHistory(qint64, qint64, int) override { return {}; }
    void insertChangeRecord(const AlarmChangeRecord&) override {}
    QVector<AlarmChangeRecord> queryChangeRecords(quint32, int) override { return {}; }
    QVector<AlarmChangeRecord> queryPendingApprovals() override { return {}; }
    void updateChangeApproval(int, bool, const QString&, const QString&) override {}
    void insertKpiSnapshot(const AlarmKpiSnapshot&) override {}
    QVector<AlarmKpiSnapshot> queryKpiHistory(qint64, qint64, int) override { return {}; }
    void purgeOldRecords(int) override {}
private:
    ILogger* m_logger = nullptr;
};
#endif
