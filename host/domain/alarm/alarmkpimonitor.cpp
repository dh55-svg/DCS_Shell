#include "alarmkpimonitor.h"
#include <QMap>
#include <algorithm>
AlarmKpiMonitor::AlarmKpiMonitor(QObject *parent)
    : QObject{parent}
{
    m_timer=new QTimer(this);
    m_timer->setInterval(60000);
    connect(m_timer,&QTimer::timeout,this,&AlarmKpiMonitor::onTick);
    m_timer->start();

}
//记录报警事件到KPI监控器中，用于后续的统计分析
void AlarmKpiMonitor::recordAlarm(quint32 tagId, const QString &tagName)
{
    QMutexLocker lock(&m_mutex);
    m_events.append({QDateTime::currentSecsSinceEpoch(), tagId, tagName});
}
//生成报警系统的KPI快照，计算并返回当前时刻报警系统的各项性能指标，包括报警频率、健康评分、最频繁报警等。
AlarmKpiSnapshot AlarmKpiMonitor::snapshot() const
{
    QMutexLocker locker(&m_mutex);
    return snapshotLocked();
}

AlarmKpiSnapshot AlarmKpiMonitor::snapshotLocked() const
{
    qint64 now=QDateTime::currentSecsSinceEpoch();
    qint64 tenMinAgo=now-600;
    qint64 oneHourAgo=now-3600;

    AlarmKpiSnapshot s;
    s.timestamp=now*1000;

    int count10min=0;
    QMap<QString,int> freqMap;
    for(const auto& e:m_events)
    {
        if(e.timestamp>=oneHourAgo)
        {
            freqMap[e.tagName]++;
            if(e.timestamp>=tenMinAgo) count10min++;
        }
    }
    s.alarmCount10min=count10min;
    // ★ 修复：avgPerHour = 近1小时总报警数（非 distinct tag count）
    int totalEvents = 0;
    for (const auto& e : m_events) {
        if (e.timestamp >= oneHourAgo) totalEvents++;
    }
    s.avgPerHour = (float)totalEvents;
    s.peakCount10min=count10min;
    s.totalActive=m_externalTotalActive;
    s.staleCount = m_externalStaleCount;
    s.shelvedCount = m_externalShelvedCount;
    s.suppressedCount = 0;
    s.staleAlarmPercent=s.totalActive>0?(s.staleCount*100/s.totalActive):0;

    QVector<QPair<QString, int>> sorted;
    for(auto it=freqMap.begin();it!=freqMap.end();++it)
        sorted.append({it.key(),it.value()});
    std::sort(sorted.begin(),sorted.end(),[](const auto&a,const auto& b){return a.second > b.second;});
    for (int i = 0; i < qMin(5, (int)sorted.size()); ++i)
        s.top5Frequent.append(sorted[i].first);

    float score=100.0f;
    if(s.alarmCount10min>m_rateThreshold10min) score-=20;
    if (s.avgPerHour > 2) score -= 10;
    if (s.staleAlarmPercent > 5) score -= 10;
    if (s.peakCount10min > m_peakThreshold10min) score -= 15;

    s.systemHealthScore = qMax(0.0f, score);

    if (score >= 90) s.healthGrade = "A";
    else if (score >= 75) s.healthGrade = "B";
    else if (score >= 60) s.healthGrade = "C";
    else if (score >= 40) s.healthGrade = "D";
    else s.healthGrade = "F";
    return s;
}

void AlarmKpiMonitor::setThresholds(int rate10min, int staleMin, int peak10min)
{
    m_rateThreshold10min = rate10min;
    m_staleThresholdMin = staleMin;
    m_peakThreshold10min = peak10min;
}

void AlarmKpiMonitor::setExternalStats(int totalActive, int staleCount, int shelvedCount)
{
    QMutexLocker lock(&m_mutex);
    m_externalTotalActive = totalActive;
    m_externalStaleCount = staleCount;
    m_externalShelvedCount = shelvedCount;
}
//统计并返回最近1小时内最频繁触发的N个报警标签
QVector<QPair<quint32, int> > AlarmKpiMonitor::topFrequent(int topN) const
{
    QMutexLocker lock(&m_mutex);
    QMap<QString, int> freqMap;
    qint64 oneHourAgo = QDateTime::currentSecsSinceEpoch() - 3600;
    for (const auto& e : m_events) {
        if (e.timestamp >= oneHourAgo) freqMap[e.tagName]++;
    }

    QVector<QPair<QString, int>> sorted;
    for (auto it = freqMap.begin(); it != freqMap.end(); ++it)
        sorted.append({it.key(), it.value()});
    std::sort(sorted.begin(), sorted.end(), [](const auto& a, const auto& b) { return a.second > b.second; });

    // ★ 修复：从 m_events 中查找 tagName 对应的 tagId
    QVector<QPair<quint32, int> > result;
    for (int i = 0; i < qMin(topN, static_cast<int>(sorted.size())); i++) {
        quint32 foundId = 0;
        for (const auto& e : m_events) {
            if (e.tagName == sorted[i].first && e.tagId != 0) { foundId = e.tagId; break; }
        }
        result.append({foundId, sorted[i].second});
    }
    return result;
}

void AlarmKpiMonitor::onTick()
{
    
    QMutexLocker lock(&m_mutex);
    pruneOldEvents(); // ① 清理过期事件
    
    auto s=snapshotLocked();// ② 生成 KPI 快照
    m_history.append(s);// ③ 存入历史（上限 1440 = 24h）
    if (m_history.size() > 1440) m_history.removeFirst();
    lock.unlock();
    emit kpiReport(s);

    if(s.alarmCount10min>m_rateThreshold10min) emit kpiThresholdExceeded("10minRate", s.alarmCount10min, m_rateThreshold10min);

}
//清理过期的报警事件数据
void AlarmKpiMonitor::pruneOldEvents()
{
    qint64 cutoff = QDateTime::currentSecsSinceEpoch() - 3600;
    while (!m_events.isEmpty() && m_events.first().timestamp < cutoff)
        m_events.removeFirst();
}
