#ifndef DATACONTROLLER_H
#define DATACONTROLLER_H

#include <QObject>
#include <memory>
#include "../pipeline/DataPipeline.h"
#include "../domain/tag/TagManager.h"
#include "../domain/alarm/AlarmEngine.h"
/**
 * @file    DataController.h
 * @brief   数据控制器 — Application 层门面，封装数据管道的启动/停止和写操作
 *
 * 架构角色：Application 层控制器（Controller），位于 Presentation 和 Pipeline 之间。
 * 不包含业务逻辑，所有操作直接委托（delegate）给 DataPipeline / TagManager / AlarmEngine：
 *
 *   Presentation（UI）→ DataController → DataPipeline → ...
 *
 * 核心职责：
 *   1. 封装 DataPipeline 生命周期（connectAll/disconnectAll）
 *   2. 封装写操作接口（writeSetPoint/writeOutput/toggleAutoMode）
 *   3. 转发数据更新、设备状态、通信状态信号给 Presentation 层
 *   4. 聚合暴露 TagManager、AlarmEngine、DataPipeline 引用供其他 Controller 使用
 *
 * 依赖：DataPipeline（管道编排器）、TagManager（位号查询）、AlarmEngine（报警引擎）、IFieldbus（总线）
 */
class DataController : public QObject
{
    Q_OBJECT
public:
    /// 构造函数：注入全部依赖并自动连接信号
    DataController(DataPipeline& pipeline, TagManager& tagMgr, AlarmEngine& alarmEngine,
                   IFieldbus& fieldbus, ILogger* logger = nullptr);

    // ---- 子组件访问器 ----
    DataPipeline& pipeline() { return m_pipeline; }          ///< 获取数据管道
    TagManager& tagManager() { return m_tagMgr; }            ///< 获取位号管理器
    AlarmEngine& alarmEngine() { return m_alarmEngine; }     ///< 获取报警引擎

    // ---- 生命周期 ----
    void connectAll();     ///< 启动数据管道（解析 + 历史 + 总线）
    void disconnectAll();  ///< 停止数据管道

    // ---- 控制接口 ----
    void writeSetPoint(quint32 tagId, float value);   ///< 写设定值
    void writeOutput(quint32 tagId, float value);     ///< 写输出值
    void toggleAutoMode(quint32 tagId);               ///< 切换手/自动模式

signals:
    void dataUpdated();                                        ///< 数据已更新（转发自 DataPipeline）
    void deviceStatusChanged(int deviceId, bool connected);    ///< 设备连接状态变化
    void commStatusChanged(bool ok);                           ///< 通信状态变化

private:
    // ---- 依赖（引用，外部注入，生命周期由 ApplicationBuilder 管理）----
    DataPipeline& m_pipeline;      ///< 数据管道
    TagManager& m_tagMgr;          ///< 位号管理器
    AlarmEngine& m_alarmEngine;    ///< 报警引擎
    IFieldbus& m_fieldbus;         ///< 现场总线接口
    ILogger* m_logger;             ///< 日志接口（可选）
};

#endif // DATACONTROLLER_H
