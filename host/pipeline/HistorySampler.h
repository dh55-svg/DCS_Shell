#ifndef HISTORYSAMPLER_H
#define HISTORYSAMPLER_H

#include <QThread>
#include <QAtomicInt>
#include <QMutex>
#include <QVector>
#include <QMap>
#include "../infrastructure/messaging/DoubleBuffer.h"
#include "plugin_interface/IHistoryRepo.h"
/**
 * @file    HistorySampler.h
 * @brief   历史采样线程 — 定时从 DoubleBuffer 采样并批量归档到数据库
 *
 * 核心职责：
 *   1. 按 sampleInterval 定时从 DoubleBuffer 读取全量快照
 *   2. 采样数据先写入 m_cache 缓冲区，按 archiveInterval 批量写入数据库
 *   3. 每个位号维护环形缓冲区（TagHistoryRing，最多 1800 点），支持近期数据内存查询
 *   4. 提供 queryTrend/queryMultiTrend：优先查内存缓存，未命中再查数据库
 *   5. 统计归档成功/失败计数，通过信号通知归档结果
 *
 * 数据流：DoubleBuffer → sampleData() → m_cache → doArchive() → IHistoryRepo::batchInsert
 *
 * 依赖：DoubleBuffer（数据源）、IHistoryRepo（持久化接口）
 */
class HistorySampler : public QThread
{
    Q_OBJECT
public:
    explicit HistorySampler(QObject *parent = nullptr);
    ~HistorySampler() override;

    // ---- 依赖注入 ----
    void setDoubleBuffer(DoubleBuffer* buffer) { m_doubleBuffer = buffer; }   ///< 设置数据源（双缓冲）
    void setHistoryRepo(IHistoryRepo* repo) { m_historyRepo = repo; }          ///< 设置持久化仓储
    void setArchiveInterval(int seconds) { m_archiveIntervalSec = seconds; }   ///< 归档周期（默认 300s）
    void setSampleInterval(int ms) { m_sampleIntervalMs = ms; }                ///< 采样周期（默认 1000ms）
    void setCacheWindow(int seconds) { m_cacheWindowSec = qBound(60, seconds, 86400); } ///< 内存缓存窗口（60-86400s）

    void stop();                                      ///< 停止线程（3 秒超时等待）
    qint64 totalArchived() const { return m_totalArchived.loadRelaxed(); }  ///< 累计归档成功条数
    qint64 totalFailed() const { return m_totalFailed.loadRelaxed(); }      ///< 累计归档失败条数

    /// 查询单个位号历史趋势（优先内存缓存，未命中则查数据库）
    QVector<HistoryRecord> queryTrend(quint32 tagId, const QDateTime& start, const QDateTime& end, int maxPoints = 10000);
    /// 批量查询多个位号历史趋势
    QMap<quint32, QVector<HistoryRecord>> queryMultiTrend(const QVector<quint32>& tagIds,
                                                          const QDateTime& start, const QDateTime& end, int maxPoints = 5000);

signals:
    void archiveCompleted(int recordCount, qint64 durationMs);  ///< 归档成功（条数 + 耗时）
    void archiveFailed(const QString& error);                    ///< 归档失败（错误信息）

protected:
    void run() override;
private:
    void sampleData();    ///< 从 DoubleBuffer 采样一批数据写入 m_cache
    bool doArchive();     ///< 将 m_cache 中数据批量写入数据库并清空
    QVector<HistoryRecord> queryFromCache(quint32 tagId, const QDateTime& start, const QDateTime& end) const; ///< 查内存环形缓冲
    void writeToRecentCache(quint32 tagId, const HistoryRecord& rec); ///< 写入位号环形缓冲

    /// 每个位号的定长环形缓冲区（1800 点 = 30min @ 1s 采样）
    struct TagHistoryRing {
        QVector<HistoryRecord> records;  ///< 定长数组
        int head = 0;                    ///< 写入位置
        int count = 0;                   ///< 已填充数量
        static constexpr int MAX_RECORDS = 1800;  ///< 最大记录数
    };

    // ---- 外部依赖 ----
    DoubleBuffer* m_doubleBuffer = nullptr;  ///< 数据源（双缓冲）
    IHistoryRepo* m_historyRepo = nullptr;   ///< 持久化仓储接口

    // ---- 批量归档缓冲区 ----
    QVector<HistoryRecord> m_cache;   ///< 待归档缓冲区
    QMutex m_cacheMutex;              ///< 缓冲区互斥锁
    qint64 m_firstRecordTime = 0;     ///< 缓冲区首条记录时间（判断归档周期）
    int m_archiveIntervalSec = 300;   ///< 归档间隔（秒）
    int m_sampleIntervalMs = 1000;    ///< 采样间隔（毫秒）

    // ---- 内存查询缓存 ----
    QMap<quint32, TagHistoryRing> m_recentHistory; ///< 每位号独立环形缓冲
    int m_cacheWindowSec = 1800;                   ///< 缓存窗口（秒，默认 30min）

    // ---- 运行状态 ----
    QAtomicInt m_running;          ///< 运行标志
    QAtomicInt m_totalArchived;    ///< 累计归档成功计数
    QAtomicInt m_totalFailed;      ///< 累计归档失败计数
};

#endif // HISTORYSAMPLER_H
