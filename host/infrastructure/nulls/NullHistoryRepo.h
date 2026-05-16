#ifndef NULLHISTORYREPO_H
#define NULLHISTORYREPO_H
#include "plugin_interface/IHistoryRepo.h"

class NullHistoryRepo : public IHistoryRepo {
public:
    void batchInsert(const QVector<HistoryRecord>&) override {}
    QVector<HistoryRecord> query(quint32, qint64, qint64, int) override { return {}; }
    void purgeOldRecords(int) override {}
};
#endif
