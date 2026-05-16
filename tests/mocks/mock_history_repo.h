#ifndef MOCK_HISTORY_REPO_H
#define MOCK_HISTORY_REPO_H
#include "plugin_interface/IHistoryRepo.h"

class MockHistoryRepo : public IHistoryRepo {
public:
    void batchInsert(const QVector<HistoryRecord>& records) override { m_records.append(records); insertCount++; }
    QVector<HistoryRecord> query(quint32 tagId, qint64, qint64, int) override {
        QVector<HistoryRecord> result;
        for (auto& r : m_records)
            if (r.tagId == tagId) result.append(r);
        return result;
    }
    void purgeOldRecords(int) override {}

    int insertCount = 0;
    QVector<HistoryRecord> m_records;
};
#endif
