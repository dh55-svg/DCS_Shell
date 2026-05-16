#ifndef ALARMCONTROLLER_H
#define ALARMCONTROLLER_H

#include <QObject>
#include "../domain/alarm/AlarmEngine.h"
/**
 * @file    AlarmController.h
 * @brief   报警控制器 — Application 层门面，封装所有报警操作接口
 *
 * 架构角色：Application 层控制器，位于 Presentation 和 Domain（AlarmEngine）之间。
 * 所有报警操作直接委托给 AlarmEngine：
 *
 *   Presentation（UI）→ AlarmController → AlarmEngine → 状态机/仓储/通知
 *
 * 核心职责：
 *   1. 封装 ISA-18.2 标准报警操作：确认/静音/搁置/抑制/旁路/标注
 *   2. 转发报警触发和计数变化信号给 Presentation 层
 *   3. 提供 AlarmEngine 引用供其他 Controller 或 ViewModel 使用
 *
 * 依赖：AlarmEngine（报警核心引擎）
 */
class AlarmController : public QObject
{
    Q_OBJECT
public:
    explicit AlarmController(AlarmEngine& engine, ILogger* logger = nullptr);

    AlarmEngine& engine() { return m_engine; }  ///< 获取报警引擎引用

    // ---- ISA-18.2 标准操作 ----
    void acknowledgeAlarm(const QString& alarmId, const QString& user = QString());  ///< 确认单条报警
    void acknowledgeAll(const QString& user = QString());                            ///< 确认所有报警
    void shelveAlarm(quint32 tagId, const QString& reason, int durationSec, const QString& user); ///< 搁置报警（定时恢复）
    void unshelveAlarm(quint32 tagId);              ///< 提前解除搁置
    void suppressAlarm(quint32 tagId, const QString& reason, const QString& user);   ///< 条件抑制
    void unsuppressAlarm(quint32 tagId);            ///< 解除抑制
    void setOutOfService(quint32 tagId, const QString& reason, const QString& user); ///< 设为停用
    void returnToService(quint32 tagId);            ///< 恢复投用
    void annotateAlarm(const QString& alarmId, const QString& text, const QString& user); ///< 添加标注
    void setSoundEnabled(bool enabled);             ///< 启用/禁用声音报警

signals:
    void alarmTriggered(const AlarmEvent& event);                     ///< 报警触发（转发自 AlarmEngine）
    void alarmCountChanged(int activeCount, int unackCount);          ///< 报警计数变化

private:
    AlarmEngine& m_engine;  ///< 报警核心引擎（引用，外部管理生命周期）
    ILogger* m_logger;      ///< 日志接口（可选）
};

#endif // ALARMCONTROLLER_H
