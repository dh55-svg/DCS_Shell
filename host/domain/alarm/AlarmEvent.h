#ifndef ALARMEVENT_H
#define ALARMEVENT_H
#include <QString>
#include <QDateTime>
#include <QStringList>
#include "../common/AlarmLimit.h"
#include "../common/DataQuality.h"
#include "../common/AlarmState.h"

// ─── 报警合理化 (ISA-18.2) ───
struct AlarmRationlization {
    QString consequence;
    QString operatorAction;
    int expectedResponseTimeSec = 300;
    QString designPhilosophy;
    QString approver;
    qint64 approvedDate = 0;
    QString correctiveAction;
    QString relatedDocuments;
    QString area;
    QString zone;
    int reviewCycleMonths = 12;
    qint64 lastReviewDate = 0;
    QString reviewer;
    bool isValid = true;
};

// ─── 报警变更记录 ───
struct AlarmChangeRecord {
    qint64 changeTime = 0;
    QString operatorName;
    quint32 tagId = 0;
    QString fieldName;
    QString oldValue;
    QString newValue;
    QString reason;
    bool approved = false;
    QString approver;
    qint64 approveTime = 0;
    bool rejected = false;
    QString rejectReason;
    QString workOrderNo;
    qint64 validUntil = 0;
    bool autoReverted = false;
    QString sessionId;
    QString workstation;
};

// ─── 报警 KPI 快照 ───
struct AlarmKpiSnapshot {
    qint64 timestamp = 0;
    int alarmCount10min = 0;
    float avgPerHour = 0.0f;
    int peakCount10min = 0;
    int staleCount = 0;
    int totalActive = 0;
    int shelvedCount = 0;
    int suppressedCount = 0;
    int floodEventCount = 0;
    float floodDurationMin = 0.0f;
    float avgAckTimeSec = 0.0f;
    int chatteringCount = 0;
    int staleAlarmPercent = 0;
    int criticalCount = 0;
    int majorCount = 0;
    int minorCount = 0;
    int advisoryCount = 0;
    QStringList top5Frequent;
    QStringList top5Stale;
    float systemHealthScore = 100.0f;
    QString healthGrade;
};

// ─── 报警事件 ───
struct AlarmEvent {
    QString alarmId;
    quint32 tagId = 0;
    QString tagName;
    QString description;
    AlarmLimit limit = AlarmLimit::Normal;
    AlarmPriority priority = AlarmPriority::Major;
    AlarmClassification classification = AlarmClassification::Process;
    AlarmState state = AlarmState::Normal;
    float triggerValue = 0.0f;
    float thresholdValue = 0.0f;
    float returnValue = 0.0f;
    qint64 triggerTime = 0;
    qint64 ackTime = 0;
    qint64 returnTime = 0;
    QString ackUser;
    QString unit;
    QString area;
    QString zone;
    AlarmRationlization rationalization;
    bool shelved = false;
    qint64 shelveUntil = 0;
    QString shelveReason;
    bool suppressed = false;
    AlarmSuppressionType suppressionType = AlarmSuppressionType::None;
    quint32 suppressionRuleId = 0;
    bool outOfService = false;
    QString annotations;
    int repeatCount = 0;
};

// ─── 报警过滤器 ───
struct AlarmFilter {
    qint64 startTime = 0;
    qint64 endTime = 0;
    quint32 tagId = 0;
    AlarmPriority minPriority = AlarmPriority::Advisory;
    AlarmPriority maxPriority = AlarmPriority::Critical;
    AlarmClassification classification = AlarmClassification::Process;
    bool filterByClassification = false;
    QString area;
    QString zone;
    QString keyword;
    bool includeShelved = false;
    bool includeSuppressed = false;
    bool includeOutOfService = false;
};

// ─── 条件抑制规则 ───
struct SuppressionRule {
    quint32 ruleId = 0;
    quint32 targetTagId = 0;
    quint32 conditionTagId = 0;
    QString conditionExpr;
    QString reason;
    bool enabled = true;
    QString createdBy;
    QString approver;
    qint64 createdTime = 0;
};

// ─── 搁置记录 ───
struct ShelveRecord {
    quint32 tagId = 0;
    qint64 shelveTime = 0;
    qint64 unshelveTime = 0;
    int durationSec = 0;
    QString reason;
    QString user;
    bool isAuto = false;
};

// ─── 报警洪泛事件 ───
struct AlarmFloodEvent {
    qint64 startTime = 0;
    qint64 endTime = 0;
    int alarmCount = 0;
    int peakRate = 0;
    int weightedCount = 0;
    QStringList topContributors;
    QString triggerCause;
    QString analyst;
    qint64 analysisTime = 0;
};

#endif
