#include "AlarmController.h"
/**
 * @file    AlarmController.cpp
 * @brief   报警控制器实现 — ISA-18.2 报警操作的薄委托层
 *
 * 所有方法均为对 AlarmEngine 的一对一委托，Controller 不维护额外状态。
 * 这种间接层的价值在于：Presentation 层不需要直接依赖 AlarmEngine 的具体
 * 实现，便于单元测试时替换 Mock 对象。
 */
// 构造函数：注入 AlarmEngine，连接信号转发至 Presentation 层
AlarmController::AlarmController(AlarmEngine& engine, ILogger* logger)
    : m_engine(engine), m_logger(logger)
{
    connect(&m_engine, &AlarmEngine::alarmTriggered, this, &AlarmController::alarmTriggered);
    connect(&m_engine, &AlarmEngine::alarmCountChanged, this, &AlarmController::alarmCountChanged);
}

void AlarmController::acknowledgeAlarm(const QString& alarmId, const QString& user) {
    m_engine.acknowledgeAlarm(alarmId, user);  ///< 确认单条 → 状态迁移 ActiveUnack→ActiveAck
}

void AlarmController::acknowledgeAll(const QString& user) {
    m_engine.acknowledgeAll(user);             ///< 批量确认所有未确认报警
}

void AlarmController::shelveAlarm(quint32 tagId, const QString& reason, int durationSec, const QString& user) {
    m_engine.shelveAlarm(tagId, reason, durationSec, user);  ///< 搁置 → 状态迁移至 Shelved
}

void AlarmController::unshelveAlarm(quint32 tagId) {
    m_engine.unshelveAlarm(tagId);             ///< 解除搁置 → 恢复原状态
}

void AlarmController::suppressAlarm(quint32 tagId, const QString& reason, const QString& user) {
    m_engine.suppressByDesign(tagId, reason, user, QString());  ///< 条件抑制
}

void AlarmController::unsuppressAlarm(quint32 tagId) {
    m_engine.unsuppressByDesign(tagId);        ///< 解除抑制
}

void AlarmController::setOutOfService(quint32 tagId, const QString& reason, const QString& user) {
    m_engine.setOutOfService(tagId, reason, user, QString());   ///< 停用 → 状态迁移至 OutOfService
}

void AlarmController::returnToService(quint32 tagId) {
    m_engine.returnToService(tagId);           ///< 恢复投用
}

void AlarmController::annotateAlarm(const QString& alarmId, const QString& text, const QString& user) {
    m_engine.annotateAlarm(alarmId, text, user);  ///< 添加操作员标注
}

void AlarmController::setSoundEnabled(bool enabled) {
    m_engine.setSoundEnabled(enabled);         ///< 控制声音报警开关
}
