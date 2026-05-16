#ifndef FLOODDETECTOR_H
#define FLOODDETECTOR_H
#include <QVector>
#include <QDateTime>
#include <QtMath>
#include "AlarmEvent.h"

class FloodDetector {
public:
    static constexpr int FLOOD_THRESHOLD = 10;
    static constexpr qint64 FLOOD_WINDOW_MS = 600000;
    static constexpr qint64 MIN_FLOOD_DURATION_MS = 60000;
    static constexpr qint64 FLOOD_COOLDOWN_MS = 30000;

    void recordAlarm(quint32 tagId, const QString& tagName,
                     AlarmPriority priority = AlarmPriority::Major) {
        qint64 now = QDateTime::currentMSecsSinceEpoch();
        if (m_windowStart == 0 || (now - m_windowStart) > FLOOD_WINDOW_MS) {
            if (m_inFlood && (now - m_floodStartTime) >= MIN_FLOOD_DURATION_MS)
                endFlood(now);
            m_windowStart = now;
            m_windowCount = 1;
            m_weightedCount = priorityWeight(priority);
            return;
        }
        m_windowCount++;
        m_weightedCount += priorityWeight(priority);
        m_lastAlarmTime = now;
        if (m_windowCount >= FLOOD_THRESHOLD && !m_inFlood) {
            m_inFlood = true;
            m_floodStartTime = now;
            AlarmFloodEvent fe;
            fe.startTime = m_windowStart;
            fe.alarmCount = m_windowCount;
            fe.peakRate = m_windowCount;
            fe.weightedCount = m_weightedCount;
            m_currentFlood = fe;
        }
        if (m_inFlood) {
            m_currentFlood.alarmCount = m_windowCount;
            m_currentFlood.peakRate = qMax(m_currentFlood.peakRate, m_windowCount);
            m_currentFlood.weightedCount = m_weightedCount;
        }
    }

    bool isInFlood() const { return m_inFlood; }
    bool isInCooldown() const {
        return !m_inFlood && m_floodEndTime > 0
               && (QDateTime::currentMSecsSinceEpoch() - m_floodEndTime) < FLOOD_COOLDOWN_MS;
    }
    AlarmFloodEvent currentFlood() const { return m_currentFlood; }
    QVector<AlarmFloodEvent> pastFloods() const { return m_pastFloods; }
    int weightedCount() const { return m_weightedCount; }

    void endFlood(qint64 now) {
        if (!m_inFlood) return;
        if (now - m_floodStartTime < MIN_FLOOD_DURATION_MS) return;
        m_currentFlood.endTime = now;
        m_pastFloods.prepend(m_currentFlood);
        if (m_pastFloods.size() > 100) m_pastFloods.removeLast();
        m_inFlood = false;
        m_floodEndTime = now;
        m_weightedCount = 0;
    }

    void checkExpired() {
        if (!m_inFlood) return;
        qint64 now = QDateTime::currentMSecsSinceEpoch();
        if (now - m_floodStartTime >= MIN_FLOOD_DURATION_MS
            && m_lastAlarmTime > 0
            && (now - m_lastAlarmTime) > FLOOD_WINDOW_MS)
            endFlood(now);
    }

private:
    static int priorityWeight(AlarmPriority p) {
        switch (p) {
        case AlarmPriority::Critical: return 5;
        case AlarmPriority::Major:    return 3;
        default:                      return 1;
        }
    }

    qint64 m_windowStart = 0;
    int m_windowCount = 0;
    int m_weightedCount = 0;
    bool m_inFlood = false;
    qint64 m_floodStartTime = 0;
    qint64 m_floodEndTime = 0;
    qint64 m_lastAlarmTime = 0;
    AlarmFloodEvent m_currentFlood;
    QVector<AlarmFloodEvent> m_pastFloods;
};
#endif
