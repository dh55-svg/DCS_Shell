#ifndef SHELVEMANAGER_H
#define SHELVEMANAGER_H
#include <QHash>
#include <QVector>
#include <QDateTime>
#include <QList>
#include "AlarmEvent.h"

class ShelveManager {
public:
    static constexpr int MAX_AUTO_SHELVE_COUNT = 3;
    static constexpr qint64 RESHELVE_WINDOW_MS = 60000;

    void shelve(quint32 tagId, int durationSec,
                const QString& reason = QString(),
                const QString& user = QString(),
                bool isAuto = false) {
        qint64 now = QDateTime::currentMSecsSinceEpoch();
        if (isAuto) {
            qint64 lastUnshelve = m_lastUnshelve.value(tagId, 0);
            if (lastUnshelve > 0 && (now - lastUnshelve) < RESHELVE_WINDOW_MS)
                m_shelveCount[tagId]++;
            else
                m_shelveCount[tagId] = 1;
        }
        ShelveRecord rec;
        rec.tagId = tagId; rec.shelveTime = now; rec.durationSec = durationSec;
        rec.reason = reason; rec.user = user; rec.isAuto = isAuto;
        m_history.prepend(rec);
        if (m_history.size() > 200) m_history.removeLast();

        if (durationSec > 0) {
            m_deadlines[tagId] = now + static_cast<qint64>(durationSec) * 1000;
            m_isAuto[tagId] = isAuto;
        } else if (durationSec < 0) {
            m_deadlines[tagId] = 1;
            m_isAuto[tagId] = isAuto;
        } else {
            m_deadlines.remove(tagId);
            m_isAuto.remove(tagId);
        }
    }

    void unshelve(quint32 tagId) {
        qint64 now = QDateTime::currentMSecsSinceEpoch();
        m_deadlines.remove(tagId);
        m_isAuto.remove(tagId);
        m_lastUnshelve[tagId] = now;
        for (int i = 0; i < m_history.size(); ++i) {
            if (m_history[i].tagId == tagId && m_history[i].unshelveTime == 0) {
                m_history[i].unshelveTime = now; break;
            }
        }
    }

    struct ExpiredResult {
        QList<quint32> normalExpired;
        QList<quint32> autoExtend;
    };

    ExpiredResult checkExpired() {
        qint64 now = QDateTime::currentMSecsSinceEpoch();
        ExpiredResult result;
        auto it = m_deadlines.begin();
        while (it != m_deadlines.end()) {
            if (it.value() > 0 && now >= it.value()) {
                quint32 tagId = it.key();
                if (m_isAuto.value(tagId, false) && m_shelveCount.value(tagId, 0) >= MAX_AUTO_SHELVE_COUNT) {
                    result.autoExtend.append(tagId);
                    it.value() = now + 600000;
                    m_shelveCount[tagId]++;
                    ++it;
                } else {
                    result.normalExpired.append(tagId);
                    it = m_deadlines.erase(it);
                    m_isAuto.remove(tagId);
                }
            } else { ++it; }
        }
        return result;
    }

    int count() const { return m_deadlines.size(); }
    int shelveCount(quint32 tagId) const { return m_shelveCount.value(tagId, 0); }
    QVector<ShelveRecord> history() const { return m_history; }
    QVector<ShelveRecord> historyForTag(quint32 tagId) const {
        QVector<ShelveRecord> result;
        for (const auto& rec : m_history)
            if (rec.tagId == tagId) result.append(rec);
        return result;
    }

private:
    QHash<quint32, qint64> m_deadlines;
    QHash<quint32, bool> m_isAuto;
    QHash<quint32, int> m_shelveCount;
    QHash<quint32, qint64> m_lastUnshelve;
    QVector<ShelveRecord> m_history;
};
#endif
