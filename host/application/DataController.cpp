#include "DataController.h"

DataController::DataController(DataPipeline& pipeline, TagManager& tagMgr, AlarmEngine& alarmEngine,
                               IFieldbus& fieldbus, ILogger* logger)
    : m_pipeline(pipeline), m_tagMgr(tagMgr), m_alarmEngine(alarmEngine), m_fieldbus(fieldbus), m_logger(logger)
{

    // 信号转发：DataPipeline → DataController → Presentation
    connect(&m_pipeline, &DataPipeline::dataUpdated, this, &DataController::dataUpdated);
    connect(&m_pipeline, &DataPipeline::deviceStatusChanged, this, &DataController::deviceStatusChanged);
    connect(&m_pipeline, &DataPipeline::commStatusChanged, this, &DataController::commStatusChanged);
}
void DataController::connectAll() { m_pipeline.start(); }     ///< 启动管道全部组件
void DataController::disconnectAll() { m_pipeline.stop(); }   ///< 停止管道全部组件

void DataController::writeSetPoint(quint32 tagId, float value) {
    m_pipeline.writeSetPoint(tagId, value);  ///< 委托给 DataPipeline
}

void DataController::writeOutput(quint32 tagId, float value) {
    m_pipeline.writeOutput(tagId, value);    ///< 委托给 DataPipeline
}

void DataController::toggleAutoMode(quint32 tagId) {
    auto tag = m_tagMgr.getTag(tagId);
    m_pipeline.setAutoMode(tagId, !tag.autoMode);  ///< ★ 修复：读取当前模式并翻转
}
