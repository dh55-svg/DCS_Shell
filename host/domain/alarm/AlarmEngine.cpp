#include "AlarmEngine.h"
#include "../tag/tagmanager.h"

#include <QFile>
AlarmEngine::AlarmEngine(IAlarmRepo& alarmRepo, TagManager* tagManager, ILogger* logger)
    : m_alarmRepo(alarmRepo), m_tagManager(tagManager), m_logger(logger)
{
    m_onDelayTimer=new QTimer(this);
    m_onDelayTimer->setInterval(500);
    connect(m_onDelayTimer, &QTimer::timeout,this,[this](){
        QMutexLocker lock(&m_onDelayMutex);
        QList<quint32> toTrigger;
        for(auto it=m_onDelayEntries.begin();it!=m_onDelayEntries.end();++it)
        {
            if(it->elapsed.hasExpired(it->onDelayMs)) toTrigger.append(it.key());
        }
        lock.unlock();
        for (quint32 tagId : toTrigger){
            OnDelayEntry entry;
            {
                QMutexLocker l(&m_onDelayMutex);
                auto it = m_onDelayEntries.find(tagId);
                if (it == m_onDelayEntries.end()) continue;
                entry = it.value();
                m_onDelayEntries.erase(it);
            }
            onOnDelayTimeout(tagId, entry.limit, entry.value, entry.threshold,
                             entry.priority, entry.classification);
        }
    });
    m_offDelayTimer = new QTimer(this);
    m_offDelayTimer->setInterval(500);
    connect(m_offDelayTimer,&QTimer::timeout,this,[this](){
        QMutexLocker lock(&m_offDelayMutex);
        QList<QPair<quint32, float>> toTrigger;
        for (auto it = m_offDelayEntries.begin(); it != m_offDelayEntries.end(); ++it) {
            if (it->elapsed.hasExpired(it->offDelayMs)) {
                toTrigger.append({it.key(), it->returnValue});
            }
        }
        lock.unlock();
        for (const auto& pair : toTrigger) {
            { QMutexLocker l(&m_offDelayMutex); m_offDelayEntries.remove(pair.first); }
            onOffDelayTimeout(pair.first, pair.second);
        }
    });
    //搁置检查定时器，用于定期检查搁置的报警是否到期
    m_shelveCheckTimer=new QTimer(this);
    m_shelveCheckTimer->setInterval(10000);
    connect(m_shelveCheckTimer, &QTimer::timeout, this, &AlarmEngine::onShelveTimerTick);

}
AlarmEngine::~AlarmEngine() {
    m_onDelayTimer->stop();
    m_offDelayTimer->stop();
    m_shelveCheckTimer->stop();
}
void AlarmEngine::initialize() {
    // QSoundEffect 在无音频设备时会导致卡死，暂时禁用
    // m_soundCritical = new QSoundEffect(this);
    // m_soundMajor = new QSoundEffect(this);
    // m_soundMinor = new QSoundEffect(this);

    m_onDelayTimer->start();
    m_offDelayTimer->start();
    m_shelveCheckTimer->start();

    m_lastKpiPersistTime=QDateTime::currentMSecsSinceEpoch();
    if(m_logger) m_logger->info("ISA-18.2 报警引擎初始化完成 (Level 1-4 + 商业化增强)");
}
//处理报警触发逻辑
void AlarmEngine::triggerAlarm(quint32 tagId, AlarmLimit limit, float triggerValue, float thresholdValue,
                               AlarmPriority priority, AlarmClassification classification, int onDelayMs) {
    QMutexLocker alarmLock(&m_activeAlarmsMutex);
    QMutexLocker onDelayLock(&m_onDelayMutex);
    QMutexLocker offDelayLock(&m_offDelayMutex);
    auto activeIt=m_activeAlarms.find(tagId);

    if(m_suppressionEngine.evaluate(tagId))
    {
        return;
    }

    if(activeIt!=m_activeAlarms.end()&&activeIt->shelved) return ;//搁置状态检查

    if (activeIt != m_activeAlarms.end() && activeIt->outOfService) return;//停用状态检查

    if (m_floodSuppressedAlarms.contains(tagId)) return;//检查是否被洪水抑制,洪水期间抑制的报警不触发
    //报警升级处理
    if(activeIt!=m_activeAlarms.end()&&activeIt->isActive())
    {
        if(limit>activeIt->limit)
        {
            AlarmLimit oldLimit = activeIt->limit;
            activeIt->limit=limit;
            activeIt->priority = priority;
            activeIt->classification = classification;
            activeIt->triggerValue = triggerValue;
            activeIt->thresholdValue = thresholdValue;
            activeIt->description = QString("%1报警升级，当前值=%2，限值=%3")
                                        .arg(limitToString(limit)).arg(triggerValue, 0, 'f', 1).arg(thresholdValue, 0, 'f', 1);
            activeIt->state=AlarmState::ActiveUnack;
            activeIt->acknowledged = false;
            activeIt->acknowledgeTime = 0;
            activeIt->repeatCount++;
            alarmLock.unlock();
            onDelayLock.unlock();
            offDelayLock.unlock();
            playAlarmSound(priority);
            emit alarmTriggered(*activeIt);
            emit alarmEscalated(tagId, oldLimit, limit);
            emit alarmCountChanged(activeAlarmCount(), unacknowledgedCount());
            return;
        }
        activeIt->triggerValue = triggerValue;
        return;
    }
    //：报警已恢复正常（RTN 状态），但操作员尚未确认恢复。此时值再次超限 → 直接重新激活为 ActiveUnack，不重新走 On-Delay流程。
    if(activeIt!=m_activeAlarms.end()&&(activeIt->state==AlarmState::ReturnToNormalunack||activeIt->state==AlarmState::ReturnToNormalack)){
        activeIt->state = AlarmState::ActiveUnack;
        activeIt->limit = limit;
        activeIt->priority = priority;
        activeIt->classification = classification;
        activeIt->triggerValue = triggerValue;
        activeIt->thresholdValue = thresholdValue;
        activeIt->triggerTime = QDateTime::currentMSecsSinceEpoch();
        activeIt->acknowledged = false;
        activeIt->acknowledgeTime = 0;
        activeIt->returnToNormalTime = 0;
        activeIt->returnAckTime = 0;
        activeIt->repeatCount++;
        activeIt->description = QString("%1报警，当前值=%2，限值=%3")
                                    .arg(limitToString(limit)).arg(triggerValue, 0, 'f', 1).arg(thresholdValue, 0, 'f', 1);
        alarmLock.unlock();
        onDelayLock.unlock();
        offDelayLock.unlock();
        playAlarmSound(priority);
        emit alarmTriggered(*activeIt);
        emit alarmCountChanged(activeAlarmCount(), unacknowledgedCount());
        return;
    }
    //如果报警已活跃，只更新触发值,不重新触发报警
    if (activeIt != m_activeAlarms.end() && activeIt->isActive()) {
        activeIt->triggerValue = triggerValue;
        return;
    }
    //新报警上电延时
    // 检查是否已在延时队列中
    auto delayIt=m_onDelayEntries.find(tagId);
    if(delayIt!=m_onDelayEntries.end())
    {
        if(limit>delayIt->limit)
        {
            delayIt->limit=limit;
            delayIt->value = triggerValue;
            delayIt->threshold = thresholdValue;
            delayIt->priority = priority;
            delayIt->classification = classification;
        }
        delayIt->value=triggerValue;
        return;
    }
    // 移除待处理的断电延时
    m_offDelayEntries.remove(tagId);
    // 创建新的上电延时条目
    OnDelayEntry entry;
    entry.limit = limit;
    entry.value = triggerValue;
    entry.threshold = thresholdValue;
    entry.priority = priority;
    entry.classification = classification;
    entry.onDelayMs = onDelayMs;
    entry.elapsed.start();
    m_onDelayEntries.insert(tagId,entry);
}
//处理上电延时到期后的实际报警触发，创建报警事件、更新系统状态并通知相关组件
void AlarmEngine::onOnDelayTimeout(quint32 tagId, AlarmLimit limit, float value, float threshold,
                                   AlarmPriority priority, AlarmClassification classification){
    QMutexLocker alarmLock(&m_activeAlarmsMutex);
    auto activeId=m_activeAlarms.find(tagId);
    if(activeId!=m_activeAlarms.end()&&activeId->isActive())
    {
        return;
    }
    //震荡检查（Chattering Check）
    //检查该标签是否频繁触发报警（震荡）
    if(m_chatteringGuard.check(tagId,3))
    {
        if (m_logger) m_logger->warn(QString("震荡报警检测: tagId=%1，自动屏蔽600秒").arg(tagId));
        emit chatteringAlarmDetected(tagId, 3);
        //如果检测到震荡，自动搁置报警10分钟（600秒）
        if (activeId != m_activeAlarms.end()) {
            activeId->shelved = true;
            activeId->shelvedTime = QDateTime::currentMSecsSinceEpoch();
            activeId->shelveReason = "震荡保护自动屏蔽";
            activeId->shelveDurationSec = 600;
            activeId->state = AlarmState::Shelved;
            m_shelveManager.shelve(tagId, 600, "震荡保护自动屏蔽", QString(), true);
        }
        alarmLock.unlock();
        emit alarmShelved(tagId, "震荡保护自动屏蔽", 600);
        emit alarmCountChanged(activeAlarmCount(), unacknowledgedCount());
        return;
    }
    QString tagName;
    int onDelayMs = 3000;
    float deadband = 1.0f;
    if(m_tagManager)
    {
        TagInf ti=m_tagManager->getTag(tagId);
        tagName = ti.tagName;
        onDelayMs = ti.onDelayMs;
        deadband = ti.deadband;
    }
    AlarmEvent event;
    event.alarmId = generateAlarmId();
    event.tagId = tagId;
    event.tagName = tagName;
    event.limit = limit;
    event.priority = priority;
    event.classification = classification;
    event.triggerValue = value;
    event.thresholdValue = threshold;
    event.triggerTime = QDateTime::currentMSecsSinceEpoch();
    event.firstTriggerTime = event.triggerTime;
    event.state = AlarmState::ActiveUnack;
    event.description = QString("%1报警，当前值=%2，限值=%3")
                            .arg(limitToString(limit)).arg(value, 0, 'f', 1).arg(threshold, 0, 'f', 1);
    //设置区域和分区信息
    if(m_tagManager)
    {
        TagInf ti = m_tagManager->getTag(tagId);
        event.area = ti.rationalization.area;
        event.zone = ti.rationalization.zone;
    }
    m_activeAlarms.insert(tagId, event);
    m_alarmHistory.prepend(event);
    if (m_alarmHistory.size() > 5000) m_alarmHistory.removeLast();

    m_alarmRepo.insertEvent(event);
    m_kpiMonitor.recordAlarm(tagId, tagName);// ④ KPI 统计
    m_floodDetector.recordAlarm(tagId, tagName, priority);// ⑤ 洪水检测
    checkFloodCondition();  // 必须在锁内调用⑥ 检查/维持洪水状态
    bool inFlood = m_floodDetector.isInFlood();
    AlarmFloodEvent floodEvent;
    if (inFlood) floodEvent = m_floodDetector.currentFlood();
    alarmLock.unlock();
    playAlarmSound(priority);
    emit alarmTriggered(event);
    emit alarmCountChanged(activeAlarmCount(), unacknowledgedCount());
    if (inFlood) {
        emit alarmFloodDetected(floodEvent);
    }

}
//处理报警清除逻辑，根据配置的断电延时参数决定是立即清除还是进入断电延时等待。
void AlarmEngine::clearAlarm(quint32 tagId, float returnValue){
    QMutexLocker alarmLock(&m_activeAlarmsMutex);
    QMutexLocker onDelayLock(&m_onDelayMutex);
    QMutexLocker offDelayLock(&m_offDelayMutex);
    //从上电延时队列中移除该标签,防止在报警清除后上电延时仍然触发
    m_onDelayEntries.remove(tagId);
    auto it=m_activeAlarms.find(tagId);
    if(it==m_activeAlarms.end()) return;
    // 搁置/洪水抑制的报警不响应清除 — 保持抑制状态
    if (it->shelved) return;
    //获取断电延时配置
    int offDelayMs=0;
    if(m_tagManager)
    {
        TagInf ti = m_tagManager->getTag(tagId);
        offDelayMs = ti.offDelayMs;
    }
    if(offDelayMs>0)
    {
        auto off=m_offDelayEntries.find(tagId);
        if(off==m_offDelayEntries.end())
        {
            OffDelayEntry entry;
            entry.returnValue = returnValue;
            entry.offDelayMs = offDelayMs;
            entry.elapsed.start();
            m_offDelayEntries.insert(tagId, entry);
        }
        return;
    }
    //立即清除处理
    it->state = AlarmState::ReturnToNormalunack;
    it->returnToNormalTime = QDateTime::currentMSecsSinceEpoch();
    it->returnValue = returnValue;
    it->active = false;
    alarmLock.unlock();
    onDelayLock.unlock();
    offDelayLock.unlock();
    emit alarmCountChanged(activeAlarmCount(), unacknowledgedCount());
}
//处理断电延时到期后的实际报警清除操作，将报警状态转换为返回正常状态
void AlarmEngine::onOffDelayTimeout(quint32 tagId, float returnValue){
    QMutexLocker alarmLock(&m_activeAlarmsMutex);
    auto it=m_activeAlarms.find(tagId);
    if(it==m_activeAlarms.end()||!it->isActive()) return;

    it->state=AlarmState::ReturnToNormalunack;
    it->returnToNormalTime = QDateTime::currentMSecsSinceEpoch();
    it->returnValue = returnValue;
    it->active = false;
    alarmLock.unlock();
    emit alarmCountChanged(activeAlarmCount(), unacknowledgedCount());
}
//检查和处理报警洪水状态，在洪水期间抑制次要报警，洪水结束后恢复被抑制的报警
// 调用者必须持有 m_activeAlarmsMutex
void AlarmEngine::checkFloodCondition(){
    if(!m_floodDetector.isInFlood())
    {
        // 局限3修复：洪水结束后分批恢复，避免瞬间大量报警同时恢复造成二次冲击
        if(!m_floodSuppressedAlarms.isEmpty())
        {
            // 冷却期内暂不恢复，等待系统稳定
            if (m_floodDetector.isInCooldown()) return;

            // 开始分批恢复流程（仅触发一次）
            if (!m_floodRecoveryPending) {
                m_floodRecoveryPending = true;
                m_pendingFloodRestoration = m_floodSuppressedAlarms;
                if (m_logger) m_logger->info(QString("报警洪水结束，%1 个被抑制的报警将在冷却期后分批恢复")
                    .arg(m_floodSuppressedAlarms.size()));
            }
        }
        return;
    }
    //洪水期间抑制非Critical报警
    int newlySuppressed = 0;
    for (auto it = m_activeAlarms.begin(); it != m_activeAlarms.end(); ++it){
        if(it->shelved) continue;
        if (it->outOfService) continue;
        if (it->isSuppressed()) continue;
        if (m_floodSuppressedAlarms.contains(it.key())) continue;

        if (it->priority != AlarmPriority::Critical) {
            m_floodSuppressedAlarms.insert(it.key(), it->state);
            it->shelved = true;
            it->shelvedTime = QDateTime::currentMSecsSinceEpoch();
            it->shelveReason = "洪水保护自动抑制";
            it->shelveDurationSec = 0;
            it->state = AlarmState::Shelved;
            newlySuppressed++;
        }
    }
    if (newlySuppressed > 0 && m_logger) {
        m_logger->warn(QString("报警洪水自动抑制: %1 个非紧急报警已暂屏蔽").arg(newlySuppressed));
    }
}
//确认报警，将报警状态从"活跃未确认"转换为"活跃已确认"，并记录确认信息
bool AlarmEngine::acknowledgeAlarm(const QString& alarmId, const QString& operatorName){
    QMutexLocker lock(&m_activeAlarmsMutex);
    for (auto it = m_activeAlarms.begin(); it != m_activeAlarms.end(); ++it) {
        if (it->alarmId == alarmId && it->state == AlarmState::ActiveUnack) {
            it->state = AlarmState::Activeack;
            it->acknowledgeTime = QDateTime::currentMSecsSinceEpoch();
            it->acknowledged = true;
            if (!operatorName.isEmpty()) it->acknowledgeUser = operatorName;
            QString id = alarmId;
            lock.unlock();
            emit alarmAcknowledged(id);
            emit alarmCountChanged(activeAlarmCount(), unacknowledgedCount());
            m_alarmRepo.updateAck(id, operatorName.isEmpty() ? "operator" : operatorName, QDateTime::currentMSecsSinceEpoch());
            return true;
        }
    }
    return false;
}
bool AlarmEngine::acknowledgeAlarmByTagId(quint32 tagId, const QString& operatorName) {
    QMutexLocker lock(&m_activeAlarmsMutex);
    auto it = m_activeAlarms.find(tagId);
    if (it == m_activeAlarms.end() || it->state != AlarmState::ActiveUnack) return false;
    it->state = AlarmState::Activeack;
    it->acknowledgeTime = QDateTime::currentMSecsSinceEpoch();
    it->acknowledged = true;
    if (!operatorName.isEmpty()) it->acknowledgeUser = operatorName;
    QString alarmId = it->alarmId;
    lock.unlock();
    emit alarmAcknowledged(alarmId);
    emit alarmCountChanged(activeAlarmCount(), unacknowledgedCount());
    return true;
}
void AlarmEngine::acknowledgeAll(const QString& operatorName) {
    QMutexLocker lock(&m_activeAlarmsMutex);
    QList<QString> acked;
    for (auto it = m_activeAlarms.begin(); it != m_activeAlarms.end(); ++it) {
        if (it->state == AlarmState::ActiveUnack) {
            it->state = AlarmState::Activeack;
            it->acknowledgeTime = QDateTime::currentMSecsSinceEpoch();
            it->acknowledged = true;
            if (!operatorName.isEmpty()) it->acknowledgeUser = operatorName;
            acked.append(it->alarmId);
        }
    }
    lock.unlock();
    for (const auto& id : acked) emit alarmAcknowledged(id);
    emit alarmCountChanged(activeAlarmCount(), unacknowledgedCount());
}
void AlarmEngine::acknowledgeReturnToNormal(const QString& alarmId) {
    QMutexLocker lock(&m_activeAlarmsMutex);
    for (auto it = m_activeAlarms.begin(); it != m_activeAlarms.end(); ++it) {
        if (it->alarmId == alarmId && it->state == AlarmState::ReturnToNormalunack) {
            it->state = AlarmState::ReturnToNormalack;
            it->returnAckTime = QDateTime::currentMSecsSinceEpoch();
            QString id = alarmId;
            m_activeAlarms.erase(it);
            lock.unlock();
            emit alarmReturnToNormalAcknowledged(id);
            emit alarmCleared(id);
            emit alarmCountChanged(activeAlarmCount(), unacknowledgedCount());
            return;
        }
    }
}
//确认返回正常状态的报警，并将报警从活跃列表中移除，完成报警的完整生命周期
void AlarmEngine::acknowledgeReturnToNormalByTagId(quint32 tagId) {
    QMutexLocker lock(&m_activeAlarmsMutex);
    auto it = m_activeAlarms.find(tagId);
    if (it == m_activeAlarms.end() || it->state != AlarmState::ReturnToNormalunack) return;
    QString alarmId = it->alarmId;
    m_activeAlarms.erase(it);
    lock.unlock();
    emit alarmReturnToNormalAcknowledged(alarmId);
    emit alarmCleared(alarmId);
    emit alarmCountChanged(activeAlarmCount(), unacknowledgedCount());
}
//批量确认所有返回正常状态的报警，将它们从活跃列表中清除
void AlarmEngine::acknowledgeAllReturnToNormal() {
    QMutexLocker lock(&m_activeAlarmsMutex);
    QList<QString> cleared;
    auto it = m_activeAlarms.begin();
    while (it != m_activeAlarms.end()) {
        if (it->state == AlarmState::ReturnToNormalunack) {
            cleared.append(it->alarmId);
            it = m_activeAlarms.erase(it);
        } else { ++it; }
    }
    lock.unlock();
    for (const auto& id : cleared) { emit alarmReturnToNormalAcknowledged(id); emit alarmCleared(id); }
    emit alarmCountChanged(activeAlarmCount(), unacknowledgedCount());
}
//搁置报警，将报警状态转换为搁置状态，暂时屏蔽该报警一段时间.
void AlarmEngine::shelveAlarm(quint32 tagId, const QString& reason, int durationSec, const QString& user){
    QMutexLocker lock(&m_activeAlarmsMutex);
    auto it=m_activeAlarms.find(tagId);
    if(it==m_activeAlarms.end()) return;
    it->shelved = true;
    it->shelvedTime = QDateTime::currentMSecsSinceEpoch();
    it->shelveReason = reason;//记录搁置原因
    it->shelveDurationSec = durationSec;//记录搁置持续时间（秒）
    if (!user.isEmpty()) it->shelveUser = user;//如果提供了用户名称，记录搁置用户
    it->state = AlarmState::Shelved;

    m_shelveManager.shelve(tagId, durationSec, reason, user, false);
    lock.unlock();
    emit alarmShelved(tagId, reason, durationSec);
    emit alarmCountChanged(activeAlarmCount(), unacknowledgedCount());
}
void AlarmEngine::shelveAlarm(quint32 tagId, int durationMin) {
    shelveAlarm(tagId, QString("操作员屏蔽"), durationMin * 60);
}
//取消搁置报警，将报警从搁置状态恢复到正常状态。
void AlarmEngine::unshelveAlarm(quint32 tagId){
    QMutexLocker lock(&m_activeAlarmsMutex);
    m_shelveManager.unshelve(tagId);
    m_floodSuppressedAlarms.remove(tagId);//从洪水抑制列表中移除
    auto it = m_activeAlarms.find(tagId);
    //检查报警是否处于搁置状态
    if (it == m_activeAlarms.end() || !it->shelved) return;

    it->shelved = false;
    it->shelvedTime = 0;
    it->shelveReason.clear();
    it->state = it->isActive() ? AlarmState::ActiveUnack : AlarmState::ReturnToNormalunack;
    lock.unlock();
    emit alarmUnshelved(tagId);
    emit alarmCountChanged(activeAlarmCount(), unacknowledgedCount());



}
//获取所有搁置状态的报警列表，用于查询和显示当前被搁置的报警
QList<AlarmEvent> AlarmEngine::shelvedAlarms() const {
    QMutexLocker lock(&m_activeAlarmsMutex);
    QList<AlarmEvent> result;
    for (const auto& e : m_activeAlarms) { if (e.shelved) result.append(e); }
    return result;
}
//通过设计抑制的方式抑制报警，将报警状态转换为设计抑制状态
void AlarmEngine::suppressByDesign(quint32 tagId, const QString& reason, const QString& user, const QString& approver){
    QMutexLocker lock(&m_activeAlarmsMutex);
    auto it = m_activeAlarms.find(tagId);
    if (it != m_activeAlarms.end()){
        it->suppressionType = AlarmSuppressionType::DesignSuppression;
        it->suppressionReason = reason;
        it->suppressionUser = user;
        it->suppressionTime = QDateTime::currentMSecsSinceEpoch();
        it->state = AlarmState::SuppressedByDesign;
    }
    lock.unlock();
    emit alarmSuppressed(tagId, reason);
    emit alarmCountChanged(activeAlarmCount(), unacknowledgedCount());
}
void AlarmEngine::suppressAlarm(quint32 tagId, const QString& reason) {
    suppressByDesign(tagId, reason, QString(), QString());
}
void AlarmEngine::unsuppressByDesign(quint32 tagId) {
    QMutexLocker lock(&m_activeAlarmsMutex);
    auto it = m_activeAlarms.find(tagId);
    if (it == m_activeAlarms.end() || it->state != AlarmState::SuppressedByDesign) return;
    it->suppressionType = AlarmSuppressionType::None;
    it->suppressionReason.clear();
    it->state = it->isActive() ? AlarmState::ActiveUnack : AlarmState::ReturnToNormalunack;
    lock.unlock();
    emit alarmUnsuppressed(tagId);
    emit alarmCountChanged(activeAlarmCount(), unacknowledgedCount());
}
void AlarmEngine::unsuppressAlarm(quint32 tagId) { unsuppressByDesign(tagId); }

void AlarmEngine::setOutOfService(quint32 tagId, const QString& reason, const QString& user, const QString& workOrderNo) {
    QMutexLocker lock(&m_activeAlarmsMutex);
    auto it = m_activeAlarms.find(tagId);
    if (it != m_activeAlarms.end()) {
        it->outOfService = true;
        it->outOfServiceReason = reason;
        it->outOfServiceUser = user;
        it->workOrderNo = workOrderNo;
        it->state = AlarmState::outOfService;
    }
    lock.unlock();
    emit alarmOutOfService(tagId, reason);
    emit alarmCountChanged(activeAlarmCount(), unacknowledgedCount());
}
void AlarmEngine::setOutOfService(quint32 tagId, const QString& reason) {
    setOutOfService(tagId, reason, QString(), QString());
}
void AlarmEngine::returnToService(quint32 tagId) {
    QMutexLocker lock(&m_activeAlarmsMutex);
    auto it=m_activeAlarms.find(tagId);
    if(it==m_activeAlarms.end()||!it->outOfService) return;
    it->outOfService = false;
    it->outOfServiceReason.clear();

    it->state=it->isActive()?AlarmState::ActiveUnack:AlarmState::ReturnToNormalunack;
    lock.unlock();
    emit alarmReturnedToService(tagId);
    emit alarmCountChanged(activeAlarmCount(), unacknowledgedCount());
}
//为报警添加操作员注释
void AlarmEngine::annotateAlarm(const QString& alarmId, const QString& annotation, const QString& user) {
    QMutexLocker lock(&m_activeAlarmsMutex);
    for (auto it = m_activeAlarms.begin(); it != m_activeAlarms.end(); ++it) {
        if (it->alarmId == alarmId) {
            it->operatorAnnotation = annotation;
            it->annotationTime = QDateTime::currentMSecsSinceEpoch();
            it->annotationUser = user;
            lock.unlock();
            emit alarmAnnotated(alarmId, annotation);
            return;
        }
    }
}
void AlarmEngine::annotateAlarm(const QString& alarmId, const QString& annotation) {
    annotateAlarm(alarmId, annotation, QString());
}
//设置报警引擎中某个标签的报警参数，并记录变更历史
bool AlarmEngine::setAlarmLimit(quint32 tagId, const QString& fieldName, float newValue,
                                const QString& operatorName, const QString& reason){
    QString oldValue;
    if(m_tagManager)
    {
        TagInf tag=m_tagManager->getTag(tagId);
        if(fieldName=="highHighLimit") oldValue = QString::number(tag.highHighLimit, 'f', 1);
        else if (fieldName == "highLimit") oldValue = QString::number(tag.highLimit, 'f', 1);
        else if (fieldName == "lowLimit") oldValue = QString::number(tag.lowLimit, 'f', 1);
        else if (fieldName == "lowLowLimit") oldValue = QString::number(tag.lowLowLimit, 'f', 1);
        else if (fieldName == "deadband") oldValue = QString::number(tag.deadband, 'f', 1);
        else if (fieldName == "onDelayMs") oldValue = QString::number(tag.onDelayMs);
        else if (fieldName == "offDelayMs") oldValue = QString::number(tag.offDelayMs);

        if (fieldName == "highHighLimit") tag.highHighLimit = newValue;
        else if (fieldName == "highLimit") tag.highLimit = newValue;
        else if (fieldName == "lowLimit") tag.lowLimit = newValue;
        else if (fieldName == "lowLowLimit") tag.lowLowLimit = newValue;
        else if (fieldName == "deadband") tag.deadband = newValue;
        else if (fieldName == "onDelayMs") tag.onDelayMs = static_cast<int>(newValue);
        else if (fieldName == "offDelayMs") tag.offDelayMs = static_cast<int>(newValue);
        m_tagManager->updateTag(tagId, tag);
    }
    AlarmChangeRecord rec;
    rec.tagId=tagId;
    rec.fieldName = fieldName;
    rec.oldValue = oldValue;
    rec.newValue = QString::number(newValue, 'f', 1);
    rec.operatorName = operatorName;
    rec.reason = reason;
    m_changeLog.recordChange(rec);
    emit alarmParameterChanged(tagId, fieldName, rec.oldValue, rec.newValue);
    emit changeRecorded(rec);
    return true;
}

bool AlarmEngine::setAlarmPriority(quint32 tagId, AlarmPriority newPriority,
                                   const QString& operatorName, const QString& reason) {
    QString oldVal;
    if (m_tagManager) {
        TagInf tag = m_tagManager->getTag(tagId);
        oldVal = QString::number(static_cast<int>(tag.priority));
        tag.priority = newPriority;
        m_tagManager->updateTag(tagId, tag);
    }

    AlarmChangeRecord rec;
    rec.tagId = tagId;
    rec.fieldName = "priority";
    rec.oldValue = oldVal;
    rec.newValue = QString::number(static_cast<int>(newPriority));
    rec.operatorName = operatorName;
    rec.reason = reason;
    m_changeLog.recordChange(rec);

    emit alarmParameterChanged(tagId, "priority", rec.oldValue, rec.newValue);
    emit changeRecorded(rec);
    return true;
}
//定时器回调函数，用于定期执行报警搁置（shelve）相关的维护任务和KPI统计更新
/*

该函数每隔一定时间被调用一次，主要执行以下任务：

    检查并解除过期的搁置报警
    统计当前报警状态指标
    检测报警洪水条件
    定期持久化KPI快照数据
*/
void AlarmEngine::onShelveTimerTick(){
    QMutexLocker lock(&m_activeAlarmsMutex);
    qint64 now = QDateTime::currentMSecsSinceEpoch();

    // Check expired shelves (局限3修复：区分正常到期和超限自动延长)
    auto expired = m_shelveManager.checkExpired();
    int totalActive = 0, staleCount = 0, shelvedCount = 0, suppressedCount = 0;
    int criticalCount = 0, majorCount = 0;
    qint64 staleCutoff = now - static_cast<qint64>(m_kpiMonitor.staleThresholdMin()) * 60 * 1000;
    for (const auto& ev : m_activeAlarms) {
        if (ev.shelved) {
            shelvedCount++;
        } else if (ev.isSuppressed()) {
            suppressedCount++;
        } else {
            totalActive++;
            if (ev.state == AlarmState::ActiveUnack && ev.triggerTime > 0 && ev.triggerTime < staleCutoff)
                staleCount++;
            if (ev.priority == AlarmPriority::Critical) criticalCount++;
            else if (ev.priority == AlarmPriority::Major) majorCount++;
        }
    }
    m_kpiMonitor.setExternalStats(totalActive, staleCount, shelvedCount);
    // 局限4修复：周期性检查洪水是否过期，无需等待新报警触发
    m_floodDetector.checkExpired();
    checkFloodCondition();

    // 局限3修复：分批恢复被洪水抑制的报警（每次最多 5 个）
    if (m_floodRecoveryPending) {
        int batchSize = qMin(5, (int)m_pendingFloodRestoration.size());
        int restored = 0;
        auto it = m_pendingFloodRestoration.begin();
        while (it != m_pendingFloodRestoration.end() && restored < batchSize) {
            auto alarmIt = m_activeAlarms.find(it.key());
            if (alarmIt != m_activeAlarms.end()) {
                alarmIt->shelved = false;
                alarmIt->shelvedTime = 0;
                alarmIt->shelveReason.clear();
                alarmIt->state = it.value();
                restored++;
            }
            m_floodSuppressedAlarms.remove(it.key());
            it = m_pendingFloodRestoration.erase(it);
        }
        if (m_pendingFloodRestoration.isEmpty()) {
            m_floodRecoveryPending = false;
            if (m_logger) m_logger->info(QString("洪水恢复完成，所有抑制报警已分批恢复"));
        }
    }

    //定期持久化KPI快照
    if (now - m_lastKpiPersistTime >= 300000) {
        m_lastKpiPersistTime = now;
        AlarmKpiSnapshot snap = m_kpiMonitor.snapshot();
        snap.suppressedCount = suppressedCount + m_floodSuppressedAlarms.size();
        snap.criticalCount = criticalCount;
        snap.majorCount = majorCount;
        lock.unlock();
        m_alarmRepo.insertKpiSnapshot(snap);
        lock.relock();
    }
    // 取出 autoExtend 列表后解锁，避免在锁内发射信号
    QList<quint32> autoExtendList = expired.autoExtend;
    QList<quint32> normalExpiredList = expired.normalExpired;
    lock.unlock();
    // 解除正常到期搁置
    for (quint32 tagId : normalExpiredList) unshelveAlarm(tagId);
    // 局限3修复：超限自动搁置 → 延长而非恢复，通知人工审查
    for (quint32 tagId : autoExtendList) {
        int sc = 0;
        {
            // ★ 修复竞态：重新加锁访问 m_activeAlarms
            QMutexLocker relock(&m_activeAlarmsMutex);
            auto it = m_activeAlarms.find(tagId);
            if (it != m_activeAlarms.end()) {
                it->shelveDurationSec = 600;
                it->shelveReason = QString("震荡保护自动屏蔽(已自动延长%1次，需人工审查阈值)")
                    .arg(m_shelveManager.shelveCount(tagId));
            }
            sc = m_shelveManager.shelveCount(tagId);
        }
        emit maxShelveExceeded(tagId, sc);
        if (m_logger) m_logger->warn(QString("tagId=%1 连续自动搁置超限(%2次)，已延长屏蔽，请审查报警阈值设置")
            .arg(tagId).arg(sc));
    }
}
QList<AlarmEvent> AlarmEngine::activeAlarms() const {
    QMutexLocker lock(&m_activeAlarmsMutex);
    QList<AlarmEvent> result;
    for (const auto& e : m_activeAlarms) { if (!e.shelved) result.append(e); }
    return result;
}
QList<AlarmEvent> AlarmEngine::unacknowledgedAlarms() const {
    QMutexLocker lock(&m_activeAlarmsMutex);
    QList<AlarmEvent> result;
    for (const auto& e : m_activeAlarms) { if (e.needsAttention()) result.append(e); }
    return result;
}

AlarmEvent AlarmEngine::alarmByTagId(quint32 tagId) const {
    QMutexLocker lock(&m_activeAlarmsMutex);
    auto it = m_activeAlarms.find(tagId);
    return (it != m_activeAlarms.end()) ? *it : AlarmEvent();
}
QList<AlarmEvent> AlarmEngine::alarmHistory(int limit) const {
    QMutexLocker lock(&m_activeAlarmsMutex);
    return m_alarmHistory.mid(0, qMin(limit, m_alarmHistory.size()));
}
//报警过滤查询函数，用于根据多种条件筛选报警列表
QList<AlarmEvent> AlarmEngine::filteredAlarms(const AlarmFilter& filter) const{
    QMutexLocker lock(&m_activeAlarmsMutex);
    QList<AlarmEvent> result;
    for(const auto& e:m_activeAlarms)
    {
        if (!filter.priorities.isEmpty() && !filter.priorities.contains(e.priority)) continue;
        if (!filter.classifications.isEmpty() && !filter.classifications.contains(e.classification)) continue;
        if (!filter.states.isEmpty() && !filter.states.contains(e.state)) continue;
        if (!filter.areas.isEmpty() && !filter.areas.contains(e.area)) continue;
        if (filter.fromTime > 0 && e.triggerTime < filter.fromTime) continue;
        if (filter.toTime > 0 && e.triggerTime > filter.toTime) continue;
        if (!filter.keyword.isEmpty() && !e.tagName.contains(filter.keyword, Qt::CaseInsensitive)
            && !e.description.contains(filter.keyword, Qt::CaseInsensitive)) continue;
        if (!filter.includeShelved && e.shelved) continue;
        if (!filter.includeSuppressed && e.isSuppressed()) continue;
        if (!filter.includeOutOfService && e.outOfService) continue;
        result.append(e);
    }
    return result;
}
int AlarmEngine::activeAlarmCount() const {
    QMutexLocker lock(&m_activeAlarmsMutex);
    int c = 0;
    for (const auto& e : m_activeAlarms) { if (!e.shelved) c++; }
    return c;
}

int AlarmEngine::activeAlarmCount(AlarmLimit limit) const {
    QMutexLocker lock(&m_activeAlarmsMutex);
    int c = 0;
    for (const auto& e : m_activeAlarms) { if (!e.shelved && e.limit == limit) c++; }
    return c;
}

int AlarmEngine::activeAlarmCount(AlarmPriority priority) const {
    QMutexLocker lock(&m_activeAlarmsMutex);
    int c = 0;
    for (const auto& e : m_activeAlarms) { if (!e.shelved && e.priority == priority) c++; }
    return c;
}
int AlarmEngine::unacknowledgedCount() const {
    QMutexLocker lock(&m_activeAlarmsMutex);
    int c = 0;
    for (const auto& e : m_activeAlarms) { if (e.needsAttention()) c++; }
    return c;
}


int AlarmEngine::suppressedCount() const {
    QMutexLocker lock(&m_activeAlarmsMutex);
    int c = 0;
    for (const auto& e : m_activeAlarms) { if (e.isSuppressed()) c++; }
    return c;
}

int AlarmEngine::outOfServiceCount() const {
    QMutexLocker lock(&m_activeAlarmsMutex);
    int c = 0;
    for (const auto& e : m_activeAlarms) { if (e.outOfService) c++; }
    return c;
}

QStringList AlarmEngine::areas() const {
    QMutexLocker lock(&m_activeAlarmsMutex);
    QStringList result;
    for (const auto& e : m_activeAlarms) {
        if (!e.area.isEmpty() && !result.contains(e.area)) result.append(e.area);
    }
    return result;
}

QList<AlarmEvent> AlarmEngine::alarmsByArea(const QString& area) const {
    QMutexLocker lock(&m_activeAlarmsMutex);
    QList<AlarmEvent> result;
    for (const auto& e : m_activeAlarms) { if (e.area == area) result.append(e); }
    return result;
}
AlarmKpiSnapshot AlarmEngine::kpiSnapshot() const { return m_kpiMonitor.snapshot(); }

QVector<QPair<quint32, int>> AlarmEngine::topFrequentAlarms(int topN) const {
    return m_kpiMonitor.topFrequent(topN);
}
QVector<AlarmFloodEvent> AlarmEngine::floodEvents() const { return m_floodDetector.pastFloods(); }

void AlarmEngine::setSoundEnabled(bool enabled) {
    m_soundEnabled = enabled;
    if (!enabled) {
        if (m_soundCritical) m_soundCritical->stop();
        if (m_soundMajor) m_soundMajor->stop();
        if (m_soundMinor) m_soundMinor->stop();
    }
}
void AlarmEngine::playAlarmSound(AlarmPriority priority) {
    if (!m_soundEnabled) return;
    QSoundEffect* effect = nullptr;
    switch (priority) {
    case AlarmPriority::Critical: effect = m_soundCritical; break;
    case AlarmPriority::Major:    effect = m_soundMajor; break;
    case AlarmPriority::Minor:
    case AlarmPriority::Advisory: effect = m_soundMinor; break;
    }
    if (effect) effect->play();
}

QString AlarmEngine::generateAlarmId() {
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    return QString("ALM_%1_%2")
        .arg(QDateTime::fromMSecsSinceEpoch(now).toString("yyyyMMddHHmmss"))
        .arg(++m_alarmCounter, 4, 10, QChar('0'));
}
QString AlarmEngine::limitToString(AlarmLimit limit) const {
    switch (limit) {
    case AlarmLimit::HighHigh:     return "高高报";
    case AlarmLimit::High:         return "高报";
    case AlarmLimit::Low:          return "低报";
    case AlarmLimit::LowLow:       return "低低报";
    case AlarmLimit::Deviation:    return "偏差报警";
    case AlarmLimit::RateOfChange: return "变化率报警";
    default:                       return "未知";
    }
}







