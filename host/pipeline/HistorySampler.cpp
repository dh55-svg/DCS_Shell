#include "HistorySampler.h"
#include <QDateTime>
#include <QElapsedTimer>
#include <exception>
/**
 * @file    HistorySampler.cpp
 * @brief   历史采样线程实现 — 定时采样、内存缓存、批量归档
 *
 * 两层缓存架构：
 *   1. m_cache：全局批量归档缓冲区，按 archiveInterval 周期批量写入数据库
 *   2. m_recentHistory：每位号独立环形缓冲（1800 点），支持近期数据快速内存查询
 *
 * 查询优先级：内存环形缓冲 → 数据库（IHistoryRepo::query）
 */
HistorySampler::HistorySampler(QObject *parent)
    : QThread{parent}
{
    m_running.storeRelaxed(0);
    m_totalArchived.storeRelaxed(0);
    m_totalFailed.storeRelaxed(0);
}
HistorySampler::~HistorySampler() { stop(); }
void HistorySampler::stop(){
    m_running.storeRelaxed(0);
    if(isRunning()){quit();wait(3000);}
}
void HistorySampler::run(){
    m_running.storeRelaxed(1);
    m_firstRecordTime=0;
    while(m_running.loadRelaxed())
    {
        sampleData();//< 1. 从 DoubleBuffer 采样
        // 2. 到达归档周期 → 批量写入数据库
        if(m_firstRecordTime>0)
        {
            qint64 now = QDateTime::currentMSecsSinceEpoch();
            if (now - m_firstRecordTime >= static_cast<qint64>(m_archiveIntervalSec) * 1000) {
                doArchive();
            }
        }
        QThread::msleep(m_sampleIntervalMs);
    }
}

void HistorySampler::sampleData(){
    if(!m_doubleBuffer) return;

    // 读取 DoubleBuffer 全量快照（shared_ptr，无锁读）
    auto snap=m_doubleBuffer->readAll();
    if(!snap||snap->empty()) return;

    qint64 now=QDateTime::currentMSecsSinceEpoch();
    QMutexLocker lock(&m_cacheMutex);

    // 遍历所有位号快照，生成 HistoryRecord 写入两层缓存
    for(const auto&[tagId,ts]:*snap)
    {
        HistoryRecord rec;
        rec.tagId=tagId;
        rec.value=ts.currentValue;
        rec.quality = static_cast<int>(ts.quality);
        rec.timestamp = now;
        m_cache.append(rec);
        writeToRecentCache(tagId, rec);     ///< 写入位号环形缓冲

    }
    // 首条记录时间戳，用于判断归档周期
    if (m_firstRecordTime == 0 && !m_cache.isEmpty()) {
        m_firstRecordTime = now;
    }
}
bool HistorySampler::doArchive()
{
    if (!m_historyRepo) return false;
    // 加锁取出所有待归档记录并清空缓冲区
    QVector<HistoryRecord> records;
    {
        QMutexLocker lock(&m_cacheMutex);
        records = std::move(m_cache);
        m_cache.clear();
        m_firstRecordTime = 0;  ///< 重置，下一轮采样重新计时
    }
    if (records.isEmpty()) return true;
    QElapsedTimer timer;
    timer.start();
    try{
        m_historyRepo->batchInsert(records);
        m_totalArchived.storeRelease(m_totalArchived.loadAcquire()+records.size());
        emit archiveCompleted(static_cast<int>(records.size()), timer.elapsed());
        return true;
    }catch (const std::exception& e) {
        m_totalFailed.storeRelaxed(m_totalFailed.loadRelaxed() + records.size());
        QString err = QString("Database write failed: %1 (%2 records, %3ms)")
                          .arg(e.what()).arg(records.size()).arg(timer.elapsed());
        qWarning() << "[HistorySampler]" << err;
        emit archiveFailed(err);
        return false;
    }catch (...) {
        m_totalFailed.storeRelaxed(m_totalFailed.loadRelaxed() + records.size());
        QString err = QString("Database write failed: unknown error (%1 records, %2ms)")
                          .arg(records.size()).arg(timer.elapsed());
        qWarning() << "[HistorySampler]" << err;
        emit archiveFailed(err);
        return false;
    }

}
// 写入位号环形缓冲区：定长 1800 点，循环覆盖
void HistorySampler::writeToRecentCache(quint32 tagId, const HistoryRecord& rec){
    auto& ring = m_recentHistory[tagId];
    if(ring.records.isEmpty())
    {
        ring.records.resize(TagHistoryRing::MAX_RECORDS);  ///< 首次访问时分配定长数组
    }
    ring.records[ring.head]=rec;
    ring.head=(ring.head+1)%TagHistoryRing::MAX_RECORDS;
    if(ring.count<TagHistoryRing::MAX_RECORDS) ring.count++;
}
// 从环形缓冲区按时间范围查询（逆序遍历，结果按时间升序）
QVector<HistoryRecord> HistorySampler::queryFromCache(quint32 tagId, const QDateTime& start, const QDateTime& end) const{
    QVector<HistoryRecord> result;
    auto it = m_recentHistory.find(tagId);
    if (it == m_recentHistory.end()) return result;


    qint64 startMs = start.toMSecsSinceEpoch();
    qint64 endMs = end.toMSecsSinceEpoch();

    const auto& ring = it.value();
    // 从最新记录向前遍历环形缓冲
    for(int i=0;i<ring.count;++i)
    {
        int idx = (ring.head - 1 - i + TagHistoryRing::MAX_RECORDS) % TagHistoryRing::MAX_RECORDS;
        const auto& rec = ring.records[idx];
        if (rec.timestamp >= startMs && rec.timestamp <= endMs) {
            result.prepend(rec);  ///< 逆序插入头部，最终升序
        }

    }
    return result;

}
// 查询趋势：优先内存缓存，未命中则查数据库
QVector<HistoryRecord> HistorySampler::queryTrend(quint32 tagId, const QDateTime& start, const QDateTime& end, int maxPoints){
    auto cached = queryFromCache(tagId, start, end);
    if (!cached.isEmpty()) return cached.mid(0, maxPoints);

    if (m_historyRepo) return m_historyRepo->query(tagId, start.toMSecsSinceEpoch(), end.toMSecsSinceEpoch(), maxPoints);
    return {};

}
// 批量查询多个位号趋势
QMap<quint32, QVector<HistoryRecord>> HistorySampler::queryMultiTrend(
    const QVector<quint32>& tagIds, const QDateTime& start, const QDateTime& end, int maxPoints) {
    QMap<quint32, QVector<HistoryRecord>> result;
    for (auto tagId : tagIds) {
        result[tagId] = queryTrend(tagId, start, end, maxPoints);
    }
    return result;
}