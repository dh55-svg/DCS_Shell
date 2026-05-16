#ifndef DATAPIPELINE_H
#define DATAPIPELINE_H

#include <QObject>
#include <QVector>
#include <memory>
#include "../infrastructure/messaging/LockFreeRingBuffer.h"
#include "plugin_interface/IFieldBus.h"
#include "../infrastructure/logging/ILogger.h"
#include "../infrastructure/logging/AuditLogger.h"
#include "DataParseThread.h"
#include "HistorySampler.h"
/**
 * @file    DataPipeline.h
 * @brief   数据处理管道 — 组合 Fieldbus/解析/历史三大子系统并协调其生命周期
 *
 * 架构角色：Pipeline 层的编排器（Orchestrator），使用组合模式将各组件串联起来：
 *
 *   IFieldbus → RingBufMessageBus → DataParseThread → DoubleBuffer → HistorySampler → IHistoryRepo
 *                (LockFreeRingBuffer)    (解析+报警)     (RCU 双缓冲)   (定时采样归档)
 *
 * 核心职责：
 *   1. 持有并管理子组件生命周期（start/stop 级联控制）
 *   2. 将外部依赖（TagManager, AlarmEngine, IHistoryRepo）注入子组件
 *   3. 转发子组件信号给上层（Presentation 层）
 *   4. 提供写控制接口：writeSetPoint（给定值）、writeOutput（输出值）、setAutoMode（手/自动切换）
 *
 * 依赖：IFieldbus（数据源）、TagManager（位号查询）、AlarmEngine（报警处理）、IHistoryRepo（历史存储）
 */
class DataPipeline : public QObject
{
    Q_OBJECT
public:
    explicit DataPipeline(QObject *parent = nullptr);
    ~DataPipeline();

    // ---- 依赖注入 ----
    void setTagManager(TagManager* mgr);              ///< 注入位号管理器（转发至 DataParseThread）
    void setAlarmEngine(AlarmEngine* engine);         ///< 注入报警引擎（转发至 DataParseThread）
    void setFieldbus(IFieldbus* bus);                 ///< 注入现场总线，并设置数据接收端为 m_messageBus
    void setHistoryRepo(IHistoryRepo* repo);          ///< 注入历史仓储（转发至 HistorySampler）
    void injectTagConfig(const QVector<TagInf>& tags); ///< 注入位号配置（转发至 DataParseThread 构建地址索引）
    void injectSource(IMessageBus* source);            ///< 注入外部消息总线（预留，当前使用内部 m_messageBus）

    // ---- 生命周期 ----
    void start();  ///< 启动全部：DataParseThread → HistorySampler → IFieldbus
    void stop();   ///< 停止全部：IFieldbus → HistorySampler → DataParseThread

    // ---- 访问器 ----
    DoubleBuffer* doubleBuffer() { return &m_doubleBuffer; }       ///< 获取双缓冲（供 UI 线程读取）
    IFieldbus* fieldbus() { return m_fieldbus; }                   ///< 获取现场总线实例
    HistorySampler* historySampler() { return &m_historySampler; } ///< 获取历史采样器（供外部查询趋势）

    // ---- 控制接口 ----
    void writeSetPoint(quint32 tagId, float value);   ///< 写设定值（工程值 → Modbus 寄存器写总线）
    void writeOutput(quint32 tagId, float value);     ///< 写输出值到 DoubleBuffer
    void setAutoMode(quint32 tagId, bool autoMode);   ///< 切换手/自动模式，手动切自动时写 SP 到总线
    void setLogger(ILogger* logger) { m_logger = logger; }       ///< 注入日志接口
    void setAuditLogger(class AuditLogger* logger) { m_auditLogger = logger; } ///< 注入审计日志
signals:
    void dataUpdated();                                                     ///< DoubleBuffer 已提交新数据
    void alarmTriggered(quint32 tagId, AlarmLimit limit, float value, float threshold); ///< 报警触发（转发自 DataParseThread）
    void alarmCleared(quint32 tagId);                                       ///< 报警清除
    void deviceStatusChanged(int deviceId, bool connected);                 ///< 设备连接状态变化
    void commStatusChanged(bool ok);                                        ///< 通信状态变化
    void writeRejected(quint32 tagId, float value, const QString& reason); ///< 写入值域校验拒绝
private:
    // ---- 数据管道组件（按数据流向排列）----
    RingBufMessageBus m_messageBus;    ///< 内部消息总线（含 LockFreeRingBuffer）
    DoubleBuffer m_doubleBuffer;       ///< RCU 双缓冲（线程安全读写分离）
    DataParseThread m_parseThread;     ///< 数据解析线程（Modbus→工程值 + 报警检查）
    HistorySampler m_historySampler;   ///< 历史采样线程（定时归档 + 趋势查询）

    // ---- 外部依赖 ----
    IFieldbus* m_fieldbus = nullptr;      ///< 现场总线接口（Modbus/Simulator/OpcUa）
    TagManager* m_tagManager = nullptr;   ///< 位号管理器
    AlarmEngine* m_alarmEngine = nullptr; ///< 报警引擎
    ILogger* m_logger = nullptr;          ///< 日志接口（可选，用于记录量程错误等）
    class AuditLogger* m_auditLogger = nullptr; ///< 审计日志接口
};

#endif // DATAPIPELINE_H
