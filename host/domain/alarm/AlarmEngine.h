#ifndef ALARMENGINE_H
#define ALARMENGINE_H

#include <QObject>
#include <QTimer>
#include <QHash>
#include <QList>
#include <QMutex>
#include <QDateTime>
#include <QElapsedTimer>
#include <QSoundEffect>
#include <memory>

#include "AlarmEvent.h"
#include "AlarmStateMachine.h"
#include "ShelveManager.h"
#include "SuppressionEngine.h"
#include "FloodDetector.h"
#include "ChatteringGuard.h"
#include "alarmchangelog.h"
#include "alarmkpimonitor.h"
#include "plugin_interface/IAlarmRepo.h"
#include "../../infrastructure/logging/ILogger.h"

class TagManager;
class DoubleBuffer;

class AlarmEngine : public QObject
{
    Q_OBJECT
public:
    AlarmEngine(IAlarmRepo& alarmRepo, TagManager* tagManager, ILogger* logger = nullptr);
    ~AlarmEngine();

    /// 初始化：加载声音资源、启动定时器
    void initialize();
    /// 绑定双缓冲数据源（用于读取实时值）
    void setDoubleBuffer(DoubleBuffer* buffer) {
    m_doubleBuffer = buffer;
    m_suppressionEngine.setDataSource(buffer);  // ★ 注入数据源，使条件抑制可用
}

    // ============== 报警触发/清除 ==============

    /**
     * @brief 触发报警（含 On-Delay、升级、重复检测）
     * @param onDelayMs On-Delay 延时（ms），信号需持续超限此时间才真正触发
     */
    void triggerAlarm(quint32 tagId, AlarmLimit limit, float triggerValue, float thresholdValue,
                      AlarmPriority priority = AlarmPriority::Major,
                      AlarmClassification classification = AlarmClassification::Process,
                      int onDelayMs = 3000);
    /// 清除报警（值恢复正常），进入 ReturnToNormal 状态
    void clearAlarm(quint32 tagId, float returnValue);
    // ============== 报警确认 ==============

    bool acknowledgeAlarm(const QString& alarmId, const QString& operatorName = QString());
    bool acknowledgeAlarmByTagId(quint32 tagId, const QString& operatorName = QString());
    void acknowledgeAll(const QString& operatorName = QString());

    void acknowledgeReturnToNormal(const QString& alarmId);
    void acknowledgeReturnToNormalByTagId(quint32 tagId);
    void acknowledgeAllReturnToNormal();

    // ============== 报警搁置 ==============

    void shelveAlarm(quint32 tagId, const QString& reason, int durationSec = 3600, const QString& user = QString());
    void shelveAlarm(quint32 tagId, int durationMin);  ///< 便捷重载，durationMin 分钟
    void unshelveAlarm(quint32 tagId);
    QList<AlarmEvent> shelvedAlarms() const;

    // ============== 报警抑制 ==============

    void suppressByDesign(quint32 tagId, const QString& reason, const QString& user, const QString& approver);
    void suppressAlarm(quint32 tagId, const QString& reason);
    void unsuppressByDesign(quint32 tagId);
    void unsuppressAlarm(quint32 tagId);
    bool addSuppressionRule(const SuppressionRule& rule) { return m_suppressionEngine.addRule(rule); }
    void removeSuppressionRule(quint32 ruleId) { m_suppressionEngine.removeRule(ruleId); }
    void setSuppressionRuleEnabled(quint32 ruleId, bool enabled) { m_suppressionEngine.setEnabled(ruleId, enabled); }
    QVector<SuppressionRule> suppressionRules() const { return m_suppressionEngine.rules(); }
    bool evaluateSuppression(quint32 tagId) const { return m_suppressionEngine.evaluate(tagId); }

    // ============== 退出服务 ==============

    void setOutOfService(quint32 tagId, const QString& reason, const QString& user, const QString& workOrderNo);
    void setOutOfService(quint32 tagId, const QString& reason);
    void returnToService(quint32 tagId);

    // ============== 注释 ==============

    void annotateAlarm(const QString& alarmId, const QString& annotation, const QString& user);
    void annotateAlarm(const QString& alarmId, const QString& annotation);

    // ============== 参数修改 ==============

    bool setAlarmLimit(quint32 tagId, const QString& fieldName, float newValue,
                       const QString& operatorName, const QString& reason);
    bool setAlarmPriority(quint32 tagId, AlarmPriority newPriority,
                          const QString& operatorName, const QString& reason);
    // ============== 查询接口 ==============

    QList<AlarmEvent> activeAlarms() const;
    QList<AlarmEvent> unacknowledgedAlarms() const;
    AlarmEvent alarmByTagId(quint32 tagId) const;
    QList<AlarmEvent> alarmHistory(int limit = 1000) const;
    QList<AlarmEvent> filteredAlarms(const AlarmFilter& filter) const;

    int activeAlarmCount() const;
    int activeAlarmCount(AlarmLimit limit) const;
    int activeAlarmCount(AlarmPriority priority) const;
    int unacknowledgedCount() const;
    int suppressedCount() const;
    int outOfServiceCount() const;
    QStringList areas() const;
    QList<AlarmEvent> alarmsByArea(const QString& area) const;

    AlarmKpiSnapshot kpiSnapshot() const;
    QVector<QPair<quint32, int>> topFrequentAlarms(int topN = 5) const;
    QVector<AlarmFloodEvent> floodEvents() const;
    QVector<ShelveRecord> shelveHistory() const { return m_shelveManager.history(); }
    QVector<ShelveRecord> shelveHistoryForTag(quint32 tagId) const { return m_shelveManager.historyForTag(tagId); }
    int shelveCount(quint32 tagId) const { return m_shelveManager.shelveCount(tagId); }
    AlarmKpiMonitor* kpiMonitor() { return &m_kpiMonitor; }
    AlarmChangeLog* changeLog() { return &m_changeLog; }

    void setSoundEnabled(bool enabled);
    bool soundEnabled() const { return m_soundEnabled; }
signals:
    // ---- 报警生命周期信号 ----
    void alarmTriggered(const AlarmEvent& event);                         ///< 报警触发
    void alarmAcknowledged(const QString& alarmId);                      ///< 报警已确认
    void alarmReturnToNormalAcknowledged(const QString& alarmId);        ///< 恢复已确认
    void alarmCleared(const QString& alarmId);                           ///< 报警已清除（归档）
    // ---- 报警操作信号 ----
    void alarmShelved(quint32 tagId, const QString& reason, int durationSec); ///< 报警已搁置
    void alarmUnshelved(quint32 tagId);                                       ///< 报警已解除搁置
    void alarmSuppressed(quint32 tagId, const QString& reason);               ///< 报警已抑制
    void alarmUnsuppressed(quint32 tagId);                                     ///< 报警已解除抑制
    void alarmOutOfService(quint32 tagId, const QString& reason);             ///< 位号退出服务
    void alarmReturnedToService(quint32 tagId);                               ///< 位号恢复服务
    // ---- 报警升级信号 ----
    void alarmEscalated(quint32 tagId, AlarmLimit oldLimit, AlarmLimit newLimit); ///< 报警升级
    void alarmParameterChanged(quint32 tagId, const QString& fieldName,
                               const QString& oldValue, const QString& newValue); ///< 参数修改
    // ---- 辅助信号 ----
    void alarmAnnotated(const QString& alarmId, const QString& annotation);  ///< 报警注释
    void chatteringAlarmDetected(quint32 tagId, int repeatCount);           ///< 检测到震荡
    void maxShelveExceeded(quint32 tagId, int shelveCount);                ///< 自动搁置超限，需人工审查
    void alarmCountChanged(int activeCount, int unackCount);                ///< 报警数量变化
    void alarmFloodDetected(const AlarmFloodEvent& floodEvent);             ///< 检测到洪水
    void changeRecorded(const AlarmChangeRecord& record);                   ///< 变更已记录
private:
    // ---- 内部处理函数 ----
    void onOnDelayTimeout(quint32 tagId, AlarmLimit limit, float value, float threshold,
                          AlarmPriority priority, AlarmClassification classification);
    void onOffDelayTimeout(quint32 tagId, float returnValue);
    void onShelveTimerTick();
    void checkFloodCondition();
    void playAlarmSound(AlarmPriority priority);
    QString generateAlarmId();
    QString limitToString(AlarmLimit limit) const;

    // ---- 依赖注入 ----
    IAlarmRepo& m_alarmRepo;    ///< 报警持久化仓库
    TagManager* m_tagManager;   ///< 位号管理器（获取位号配置）
    ILogger* m_logger;          ///< 日志接口

    // ---- 报警存储 ----
    QHash<quint32, AlarmEvent> m_activeAlarms;  ///< 活跃报警表（tagId → event）
    QList<AlarmEvent> m_alarmHistory;           ///< 报警历史（最近 5000 条）
    // ---- On-Delay 机制（防抖动） ----
    struct OnDelayEntry {
        AlarmLimit limit;
        float value;
        float threshold;
        AlarmPriority priority;
        AlarmClassification classification;
        int onDelayMs = 3000;
        QElapsedTimer elapsed;
    };
    QHash<quint32, OnDelayEntry> m_onDelayEntries;
    QTimer* m_onDelayTimer = nullptr;
    // ---- Off-Delay 机制（防瞬时恢复） ----
    struct OffDelayEntry {
        float returnValue;
        int offDelayMs = 0;
        QElapsedTimer elapsed;
    };
    QHash<quint32, OffDelayEntry> m_offDelayEntries;
    QTimer* m_offDelayTimer = nullptr;
    // ---- 子组件 ----
    ShelveManager m_shelveManager;                  ///< 搁置管理
    QTimer* m_shelveCheckTimer = nullptr;           ///< 搁置到期检查定时器（10s）
    SuppressionEngine m_suppressionEngine;          ///< 条件抑制引擎
    FloodDetector m_floodDetector;                  ///< 洪水检测器
    ChatteringGuard m_chatteringGuard;              ///< 震荡保护
    QHash<quint32, AlarmState> m_floodSuppressedAlarms; ///< 洪水期间被抑制的报警及原始状态
    bool m_floodRecoveryPending = false;                   ///< 洪水结束后等待分批恢复
    QHash<quint32, AlarmState> m_pendingFloodRestoration;  ///< 待分批恢复的报警列表
    // ---- 声音 ----
    QSoundEffect* m_soundCritical = nullptr;
    QSoundEffect* m_soundMajor = nullptr;
    QSoundEffect* m_soundMinor = nullptr;
    bool m_soundEnabled = true;
    // ---- KPI 与审计 ----
    AlarmKpiMonitor m_kpiMonitor;
    AlarmChangeLog m_changeLog;
    // ---- 其他 ----
    DoubleBuffer* m_doubleBuffer = nullptr;  ///< 双缓冲数据源（可选绑定）
    int m_alarmCounter = 0;                  ///< 报警序号（生成 alarmId 用）
    qint64 m_lastKpiPersistTime = 0;        ///< 上次 KPI 持久化时间
    // ---- 细粒度锁（按数据结构拆分，消除定时器回调与外部 API 的锁竞争）----
    // 获取顺序（避免死锁）：m_activeAlarmsMutex → m_onDelayMutex → m_offDelayMutex
    mutable QMutex m_activeAlarmsMutex;  ///< 保护 m_activeAlarms / m_alarmHistory / m_floodSuppressedAlarms 等
    mutable QMutex m_onDelayMutex;       ///< 仅保护 m_onDelayEntries（On-Delay 500ms 定时器）
    mutable QMutex m_offDelayMutex;      ///< 仅保护 m_offDelayEntries（Off-Delay 500ms 定时器）
};

#endif // ALARMENGINE_H
