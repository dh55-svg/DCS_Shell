#include "DataParseThread.h"
#include "../domain/tag/tagmanager.h"
#include "../domain/alarm/AlarmEngine.h"
#include <QDebug>
#include <QElapsedTimer>
#include <QDateTime>
#include <cmath>
/**
 * @file    DataParseThread.cpp
 * @brief   数据解析线程实现 — 批量消费、线性映射、死区比较、报警检查
 *
 * 处理流程（每条 Modbus 原始数据）：
 *   1. (serverAddr<<16)|regAddr 查表得到 tagId
 *   2. registerToValue：0-65535 → engLow-engHigh 线性映射
 *   3. validateRateOfChange：3 倍 ROC 限值尖峰过滤
 *   4. 写入 DoubleBuffer（TagSnapshot）
 *   5. checkAlarmLimits：HH→H→L→LL 四级报警 + 恢复检测
 *   6. DeviationChecker：PV vs SP 偏差超标报警
 *   7. RateOfChangeChecker：变化率超标报警
 */
DataParseThread::DataParseThread(QObject *parent)
    : QThread{parent}
{
    m_running.storeRelaxed(0);
}

DataParseThread::~DataParseThread()
{
    stop();
}
void DataParseThread::setRingBuffer(LockFreeRingBuffer<RawModbusData, 8192>* queue) { m_ringBuffer = queue; }
void DataParseThread::setDoubleBuffer(DoubleBuffer* buffer) { m_doubleBuffer = buffer; }

void DataParseThread::setTagConfig(const QVector<TagInf>& tags){
    m_tags=tags;
    m_tagIdLookup.clear();
    for(const auto& tag:tags)
    {
        quint32 key=(static_cast<quint32>(tag.modbusServerAddr)<<16|static_cast<quint32>(tag.modbusRegAddr));
        m_tagIdLookup[key]=tag.tagId;
    }
}
void DataParseThread::stop(){
    m_running.storeRelaxed(0);
    if(isRunning())
    {
        quit();
        wait(3000);
    }
}
void DataParseThread::run(){
    m_running.storeRelaxed(1);
    m_lastSwapTime=QDateTime::currentMSecsSinceEpoch();

    if (!m_ringBuffer || !m_doubleBuffer) {
        m_running.storeRelaxed(0);
        return;
    }
    while(m_running.loadRelaxed())
    {
        // 批量无锁出队，每次最多 256 条
        std::vector<RawModbusData> batch;
        size_t count=m_ringBuffer->dequeueBatch(batch,256);
        if(count>0)
        {
            processBatch(batch);
        }
        // 按 swapInterval 周期 commit DoubleBuffer 并通知 UI
        qint64 now=QDateTime::currentMSecsSinceEpoch();
        if(now-m_lastSwapTime>=m_swapIntervalMs)
        {
            m_doubleBuffer->commit();
            m_lastSwapTime = now;
            emit dataUpdated();
        }
        QThread::msleep(m_processIntervalMs);
    }
}
void DataParseThread::processBatch(const std::vector<RawModbusData>& batch){
    for (const auto& raw : batch){
        for(int i=0;i<raw.count;i++)
        {
            // 1. 地址 → tagId 查表
            quint32 key = (static_cast<quint32>(raw.serverAddr) << 16) | static_cast<quint32>(raw.startAddr + i);
            auto idIt = m_tagIdLookup.find(key);
            if (idIt == m_tagIdLookup.end()) continue;  ///< 未配置的寄存器地址，跳过

            quint32 tagId = idIt.value();
            TagInf cfg;
            if(m_tagManager) cfg=m_tagManager->getTag(tagId);
            else cfg = TagInf{};
            // 2. 寄存器值 → 工程值线性映射
            float engValue = registerToValue(raw.values[i], cfg.engLow, cfg.engHigh);

            if(validateRateOfChange(tagId,engValue,cfg)) continue;

            // 4. 写入 DoubleBuffer 供 UI/历史线程读取
            DoubleBuffer::Snapshot snap;
            snap.tagId = tagId;
            snap.currentValue = engValue;
            snap.timestamp = QDateTime::currentMSecsSinceEpoch();
            snap.quality = DataQuality::Good;
            m_doubleBuffer->write(tagId, snap);

            m_prevValues[tagId] = engValue;

            // 5. 四级报警限检查（HH/H/L/LL + 恢复）
            checkAlarmLimits(tagId, engValue, cfg);

            // 6. 偏差检查（PV vs SP）
            if (cfg.deviationEnabled && m_tagManager) {
                // SP 位号查找：SP 寄存器 = PV 寄存器 + 1（同设备同从站）
                quint32 spTagId = m_tagManager->findTagByModbusAddr(cfg.modbusServerAddr, cfg.modbusRegAddr + 1);
                float sp = (spTagId != 0) ? m_prevValues.value(spTagId, 0.0f) : 0.0f;
                if (DeviationChecker::exceedsDeviation(engValue, sp, cfg.deviationLimit)) {
                    if (m_alarmEngine)
                        m_alarmEngine->triggerAlarm(tagId, AlarmLimit::Deviation, engValue, sp + cfg.deviationLimit,
                                                    AlarmPriority::Major, AlarmClassification::Process);
                }
            }

            // 7. 变化率报警
            if (cfg.rateOfChangeEnabled && m_rocChecker.exceedsLimit(tagId, engValue, cfg)) {
                if (m_alarmEngine)
                    m_alarmEngine->triggerAlarm(tagId, AlarmLimit::RateOfChange, engValue, cfg.rateOfChangeLimit,
                                                AlarmPriority::Major, AlarmClassification::Process);
            }

        }
    }
}
// 线性映射：Modbus 寄存器原始值 (0-65535) → 工程值 (engLow-engHigh)
float DataParseThread::registerToValue(quint16 raw, float engLow, float engHigh){
    float range = engHigh - engLow;
    if (range <= 0) return engLow;  ///< 量程无效时返回低限
    return engLow + (static_cast<float>(raw) / 65535.0f) * range;
}
// 尖峰检测：瞬时变化率超过 3 倍 ROC 限值时视为传感器毛刺
bool DataParseThread::validateRateOfChange(quint32 tagId, float newValue, const TagInf& cfg) {
    if (cfg.rateOfChangeLimit <= 0.0f) return false;  ///< 未启用 ROC 过滤
    auto it = m_prevValues.find(tagId);
    if (it == m_prevValues.end()) return false;       ///< 首次采样，无前值可比较

    float dt = static_cast<float>(m_processIntervalMs) / 1000.0f;
    if (dt <= 0) return false;
    float rate = std::abs(newValue - it.value()) / dt;
    return rate > cfg.rateOfChangeLimit * 3.0f; // spike detection: 3x normal ROC limit
}
// 四级报警检查：HH→H→L→LL 优先级递减，命中任一级即停止；全未命中则检查恢复
void DataParseThread::checkAlarmLimits(quint32 tagId, float value, const TagInf& tag) {
    if (!tag.alarmEnabled || !m_alarmEngine) return;
    float prevValue = m_prevValues.value(tagId, value);

    // 按优先级从高到低：HighHigh > High > Low > LowLow
    if (tag.highHighEnabled && DeadbandFilter::exceedsDeadbaud(value, tag.highHighLimit, tag.deadband, AlarmLimit::HighHigh, prevValue)) {
        m_alarmEngine->triggerAlarm(tagId, AlarmLimit::HighHigh, value, tag.highHighLimit, AlarmPriority::Critical);
    } else if (tag.highEnabled && DeadbandFilter::exceedsDeadbaud(value, tag.highLimit, tag.deadband, AlarmLimit::High, prevValue)) {
        m_alarmEngine->triggerAlarm(tagId, AlarmLimit::High, value, tag.highLimit, AlarmPriority::Major);
    } else if (tag.lowEnabled && DeadbandFilter::exceedsDeadbaud(value, tag.lowLimit, tag.deadband, AlarmLimit::Low, prevValue)) {
        m_alarmEngine->triggerAlarm(tagId, AlarmLimit::Low, value, tag.lowLimit, AlarmPriority::Major);
    } else if (tag.lowLowEnabled && DeadbandFilter::exceedsDeadbaud(value, tag.lowLowLimit, tag.deadband, AlarmLimit::LowLow, prevValue)) {
        m_alarmEngine->triggerAlarm(tagId, AlarmLimit::LowLow, value, tag.lowLowLimit, AlarmPriority::Critical);
    } else {
        // 所有已启用的报警限均已恢复正常 → 清除报警
        bool hhOK = !tag.highHighEnabled || DeadbandFilter::returnsToNormal(value, tag.highHighLimit, tag.deadband, AlarmLimit::HighHigh);
        bool hOK = !tag.highEnabled || DeadbandFilter::returnsToNormal(value, tag.highLimit, tag.deadband, AlarmLimit::High);
        bool lOK = !tag.lowEnabled || DeadbandFilter::returnsToNormal(value, tag.lowLimit, tag.deadband, AlarmLimit::Low);
        bool llOK = !tag.lowLowEnabled || DeadbandFilter::returnsToNormal(value, tag.lowLowLimit, tag.deadband, AlarmLimit::LowLow);

        if (hhOK && hOK && lOK && llOK) {
            m_alarmEngine->clearAlarm(tagId, value);
        }
    }
}
