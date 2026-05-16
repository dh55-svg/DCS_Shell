#include "DataPipeline.h"
#include "../domain/tag/tagmanager.h"
#include "../domain/alarm/AlarmEngine.h"
#include "../infrastructure/security/Security.h"
#include <QDateTime>
#include <QDebug>
DataPipeline::DataPipeline(QObject *parent)
    : QObject{parent}
{
    m_parseThread.setRingBuffer(m_messageBus.ringBuffer());//< 解析线程消费环形缓冲数据
    m_parseThread.setDoubleBuffer(&m_doubleBuffer);
    m_historySampler.setDoubleBuffer(&m_doubleBuffer);


    // 信号转发：将 DataParseThread 的信号向上转发给 Presentation 层
    connect(&m_parseThread, &DataParseThread::dataUpdated, this, &DataPipeline::dataUpdated);
    connect(&m_parseThread, &DataParseThread::alarmTriggered, this, &DataPipeline::alarmTriggered);
}
DataPipeline::~DataPipeline() { stop(); }

void DataPipeline::setTagManager(TagManager* mgr){
    m_tagManager=mgr;
    m_parseThread.setTagManager(mgr);
}
void DataPipeline::setAlarmEngine(AlarmEngine* engine) {
    m_alarmEngine = engine;
    m_parseThread.setAlarmEngine(engine);  ///< 转发给解析线程
}

void DataPipeline::setFieldbus(IFieldbus* bus) {
    m_fieldbus = bus;
    if (bus) bus->setDataSink(&m_messageBus);  ///< 现场总线数据写入 m_messageBus
}

void DataPipeline::setHistoryRepo(IHistoryRepo* repo) {
    m_historySampler.setHistoryRepo(repo);  ///< 转发给历史采样器
}

void DataPipeline::injectTagConfig(const QVector<TagInf>& tags) {
    m_parseThread.setTagConfig(tags);  ///< 位号配置传递给解析线程构建地址索引
}
void DataPipeline::injectSource(IMessageBus* source) {
    Q_UNUSED(source);
    // 当前数据源是 IFieldbus，写入内部 m_messageBus
}
void DataPipeline::start() {
    m_parseThread.start();           ///< 1. 启动数据解析线程
    m_historySampler.start();        ///< 2. 启动历史采样线程
    if (m_fieldbus) {
        m_fieldbus->startAll();  ///< 3. 启动现场总线通信
    }
}
void DataPipeline::stop() {
    m_parseThread.stop();            ///< 1. 停止数据解析
    m_historySampler.stop();         ///< 2. 停止历史采样
    if (m_fieldbus) m_fieldbus->stopAll();   ///< 3. 停止现场总线
}


// 写设定值：工程值 → Modbus 寄存器值 → 写总线
void DataPipeline::writeSetPoint(quint32 tagId, float value) {
    // ── 安全检查：值域校验 ──
    if (m_tagManager) {
        auto tag = m_tagManager->getTag(tagId);
        if (!Security::validateWriteValue(value, tag.engLow, tag.engHigh)) {
            QString reason = QString("value %1 out of range [%2, %3]")
                .arg(value).arg(tag.engLow).arg(tag.engHigh);
            if (m_logger) m_logger->warn(QString("[SECURITY] writeSetPoint rejected: tagId=%1 %2")
                .arg(tagId).arg(reason));
            emit writeRejected(tagId, value, reason);
            return;
        }
    }

    // ── 操作审计：记录写入操作 ──
    if (m_auditLogger) {
        m_auditLogger->record("operator", "write",
            QString("tagId=%1").arg(tagId),
            QString("setPoint=%1").arg(value));
    }

    // 写入 DoubleBuffer 供 UI 读取
    DoubleBuffer::Snapshot snap;
    snap.tagId = tagId;
    snap.setPoint = value;
    snap.timestamp = QDateTime::currentMSecsSinceEpoch();
    m_doubleBuffer.write(tagId, snap);

    // 工程值反算为 Modbus 寄存器值并写入现场总线
    if (m_tagManager && m_fieldbus) {
        auto tag = m_tagManager->getTag(tagId);
        float range = tag.engHigh - tag.engLow;
        if (range <= 0.0f) {
            if (m_logger) m_logger->error(QString("[DataPipeline] writeSetPoint tagId=%1 量程配置错误: engHigh=%2 engLow=%3")
                .arg(tagId).arg(tag.engHigh).arg(tag.engLow));
            return;
        }
        quint16 rawVal = static_cast<quint16>(((value - tag.engLow) / range) * 65535.0f);
        m_fieldbus->writeRegister(tag.modbusDeviceId, tag.modbusRegAddr, rawVal);
    }
}
// 写输出值到 DoubleBuffer
void DataPipeline::writeOutput(quint32 tagId, float value) {
    DoubleBuffer::Snapshot snap;
    snap.tagId = tagId;
    snap.outputValue = value;
    snap.timestamp = QDateTime::currentMSecsSinceEpoch();
    m_doubleBuffer.write(tagId, snap);
}
// 手/自动模式切换：手动切自动时，将当前 SP 写入总线
void DataPipeline::setAutoMode(quint32 tagId, bool autoMode) {
    if (m_tagManager) {
        auto tag = m_tagManager->getTag(tagId);
        if (tag.tagId != tagId) return;  ///< 位号不存在
        tag.autoMode = autoMode;
        m_tagManager->updateTag(tagId, tag);

        // 切回自动模式时，将当前设定值下发至现场总线
        if (autoMode && m_fieldbus) {
            auto snap = m_doubleBuffer.readTag(tagId);
            float sp = snap.setPoint;

            // ── 安全检查：值域校验 ──
            if (!Security::validateWriteValue(sp, tag.engLow, tag.engHigh)) {
                QString reason = QString("setPoint %1 out of range [%2, %3]")
                    .arg(sp).arg(tag.engLow).arg(tag.engHigh);
                if (m_logger) m_logger->warn(QString("[SECURITY] setAutoMode write rejected: tagId=%1 %2")
                    .arg(tagId).arg(reason));
                emit writeRejected(tagId, sp, reason);
                return;
            }

            // ── 操作审计：记录手/自动切换 ──
            if (m_auditLogger) {
                m_auditLogger->record("operator", "mode",
                    QString("tagId=%1").arg(tagId),
                    autoMode ? "auto" : "manual");
            }

            float range = tag.engHigh - tag.engLow;
            if (range <= 0.0f) {
                if (m_logger) m_logger->error(QString("[DataPipeline] setAutoMode tagId=%1 量程配置错误: engHigh=%2 engLow=%3")
                    .arg(tagId).arg(tag.engHigh).arg(tag.engLow));
                return;
            }
            quint16 rawSp = static_cast<quint16>(((sp - tag.engLow) / range) * 65535.0f);
            m_fieldbus->writeRegister(tag.modbusDeviceId, tag.modbusRegAddr + 1, rawSp);
        }
    }
}
