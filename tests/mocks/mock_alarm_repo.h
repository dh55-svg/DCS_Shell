#ifndef MOCK_ALARM_REPO_H
#define MOCK_ALARM_REPO_H
#include "plugin_interface/IAlarmRepo.h"

class MockAlarmRepo : public IAlarmRepo {
public:
    void insertEvent(const AlarmEvent& e) override { m_events.append(e); insertCount++; }
    void batchInsertEvents(const QVector<AlarmEvent>& events) override { m_events.append(events); }
    void updateAck(const QString&, const QString&, qint64) override { ackCount++; }
    void updateEvent(const QString&, const QString&, const QString&, qint64) override {}
    QVector<AlarmEvent> queryActive() override { return m_events; }
    QVector<AlarmEvent> queryEvents(const AlarmFilter&, int) override { return m_events; }
    QVector<AlarmEvent> queryHistory(qint64, qint64, int) override { return m_events; }
    void insertChangeRecord(const AlarmChangeRecord&) override {}
    QVector<AlarmChangeRecord> queryChangeRecords(quint32, int) override { return {}; }
    QVector<AlarmChangeRecord> queryPendingApprovals() override { return {}; }
    void updateChangeApproval(int, bool, const QString&, const QString&) override {}
    void insertKpiSnapshot(const AlarmKpiSnapshot&) override {}
    QVector<AlarmKpiSnapshot> queryKpiHistory(qint64, qint64, int) override { return {}; }
    void purgeOldRecords(int) override {}

    int insertCount = 0;
    int ackCount = 0;
    QVector<AlarmEvent> m_events;
};
#endif
