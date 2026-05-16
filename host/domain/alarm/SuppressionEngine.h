#ifndef SUPPRESSIONENGINE_H
#define SUPPRESSIONENGINE_H
#include <QVector>
#include <QMutex>
#include <QMutexLocker>
#include <QRegularExpression>
#include <QtMath>
#include "AlarmEvent.h"
#include "../../infrastructure/messaging/DoubleBuffer.h"

class SuppressionEngine {
public:
    void setDataSource(DoubleBuffer* buffer) { m_dataSource = buffer; }

    bool addRule(const SuppressionRule& rule) {
        QMutexLocker lock(&m_mutex);
        for (auto& r : m_rules)
            if (r.ruleId == rule.ruleId) return false;
        m_rules.append(rule);
        return true;
    }

    void removeRule(quint32 ruleId) {
        QMutexLocker lock(&m_mutex);
        m_rules.erase(std::remove_if(m_rules.begin(), m_rules.end(),
            [ruleId](const SuppressionRule& r) { return r.ruleId == ruleId; }), m_rules.end());
    }

    void setEnabled(quint32 ruleId, bool enabled) {
        QMutexLocker lock(&m_mutex);
        for (auto& r : m_rules)
            if (r.ruleId == ruleId) { r.enabled = enabled; return; }
    }

    QVector<SuppressionRule> rules() const {
        QMutexLocker lock(&m_mutex);
        return m_rules;
    }

    bool evaluate(quint32 tagId) const {
        QMutexLocker lock(&m_mutex);
        for (const auto& r : m_rules) {
            if (!r.enabled || r.targetTagId != tagId) continue;
            if (r.conditionTagId != 0 && m_dataSource && !r.conditionExpr.isEmpty()) {
                auto snap = m_dataSource->readTag(r.conditionTagId);
                if (!evaluateCondition(r.conditionExpr, snap.currentValue))
                    continue;
            }
            return true;
        }
        return false;
    }

private:
    static bool evaluateCondition(const QString& expr, float value) {
        static QRegularExpression re(R"re(value\s*(==|!=|>=|<=|>|<)\s*([\d.\-]+))re");
        auto m = re.match(expr);
        if (!m.hasMatch()) return true;
        QString op = m.captured(1);
        float threshold = m.captured(2).toFloat();
        if (op == "==") return qFuzzyCompare(value, threshold);
        if (op == "!=") return !qFuzzyCompare(value, threshold);
        if (op == ">=") return value >= threshold;
        if (op == "<=") return value <= threshold;
        if (op == ">")  return value > threshold;
        if (op == "<")  return value < threshold;
        return true;
    }

    QVector<SuppressionRule> m_rules;
    DoubleBuffer* m_dataSource = nullptr;
    mutable QMutex m_mutex;
};
#endif
