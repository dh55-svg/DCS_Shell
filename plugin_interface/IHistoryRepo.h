#ifndef IHISTORYREPO_H
#define IHISTORYREPO_H
#include <QVector>
#include <QString>

struct HistoryRecord {
    quint32 tagId = 0;
    double value = 0.0;
    int quality = 0;
    qint64 timestamp = 0;
};

class IHistoryRepo {
public:
    virtual ~IHistoryRepo() = default;
    virtual void batchInsert(const QVector<HistoryRecord>& records) = 0;
    virtual QVector<HistoryRecord> query(quint32 tagId, qint64 startTime, qint64 endTime, int maxPoints) = 0;
    virtual void purgeOldRecords(int keepDays) = 0;
};

#define IHistoryRepo_iid "com.dcsshell.IHistoryRepo"
#endif
