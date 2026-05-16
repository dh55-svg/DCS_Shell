#ifndef DATAPARSETHREAD_H
#define DATAPARSETHREAD_H
#include <QThread>
#include <QAtomicInt>
#include <QHash>
#include <QVector>
#include "../infrastructure/messaging/DoubleBuffer.h"
#include "../infrastructure/messaging/LockFreeRingBuffer.h"
#include "../domain/tag/TagInfo.h"
#include "../domain/tag/DeadbandFilter.h"
#include "../domain/tag/DeviationChecker.h"
#include "../domain/tag/RateOfChangeChecker.h"
class TagManager;
class AlarmEngine;
class DataParseThread : public QThread
{
    Q_OBJECT
public:
    explicit DataParseThread(QObject *parent = nullptr);
    ~DataParseThread() override;

    // ---- 依赖注入 ----
    void setRingBuffer(LockFreeRingBuffer<RawModbusData, 8192>* queue); ///< 设置原始数据源（无锁环形缓冲）
    void setDoubleBuffer(DoubleBuffer* buffer);                         ///< 设置数据输出目标（双缓冲）
    void setTagConfig(const QVector<TagInf>& tags);                    ///< 注入位号配置并构建地址索引
    void setTagManager(TagManager* mgr) { m_tagManager = mgr; }        ///< 设置位号管理器（查询组态）
    void setAlarmEngine(AlarmEngine* engine) { m_alarmEngine = engine; } ///< 设置报警引擎（触发/清除报警）

    // ---- 运行参数 ----
    void setProcessInterval(int ms) { m_processIntervalMs = ms; }  ///< 处理循环间隔（默认 20ms）
    void setSwapInterval(int ms) { m_swapIntervalMs = ms; }        ///< DoubleBuffer commit 间隔（默认 50ms）

    void stop();                                      ///< 停止线程（3 秒超时等待）
    DoubleBuffer* doubleBuffer() const { return m_doubleBuffer; }

    /// 测试入口：直接调用 processBatch（避免启动线程）
    void processBatchTest(const std::vector<RawModbusData>& batch) { processBatch(batch); }

signals:
    void dataUpdated();                              ///< DoubleBuffer 已提交新数据
    void alarmTriggered(quint32 tagId, AlarmLimit limit, float value, float threshold); ///< 检测到报警
protected:
    void run() override;
private:
    //处理一批原始 Modbus 数据：解析→工程值→写入 DoubleBuffer→报警检查
    void processBatch(const std::vector<RawModbusData>& batch);

    // Modbus 寄存器值 → 工程值线性映射（0-65535 → engLow-engHigh）
    float registerToValue(quint16 raw, float engLow, float engHigh);
    // 变化率尖峰检测（3 倍 ROC 限值，用于过滤传感器瞬时毛刺）
    bool validateRateOfChange(quint32 tagId, float newValue, const TagInf& cfg);

    // 遍历 HH/H/L/LL 四条报警限，与前一值做死区比较后触发或清除报警
    void checkAlarmLimits(quint32 tagId, float value, const TagInf& tag);

    // ---- 数据源与输出 ----
    LockFreeRingBuffer<RawModbusData, 8192>* m_ringBuffer = nullptr;  ///< 原始数据源
    DoubleBuffer* m_doubleBuffer = nullptr;
    // ---- 外部依赖 ----
    TagManager* m_tagManager = nullptr;    ///< 位号管理器
    AlarmEngine* m_alarmEngine = nullptr; ///< 报警引擎
    // ---- 地址索引 ----
    QHash<quint32, quint32> m_tagIdLookup; ///< (serverAddr<<16)|regAddr → tagId 快速查表
    QVector<TagInf> m_tags;               ///< 位号配置副本
    // ---- 算法组件 ----
    RateOfChangeChecker m_rocChecker;          ///< 变化率校验器
    QHash<quint32, float> m_prevValues;        ///< 每个位号的上一次采样值（死区比较用）
    // ---- 运行状态 ----
    QAtomicInt m_running;                ///< 运行标志（原子操作，线程安全）
    int m_processIntervalMs = 20;        ///< 处理循环周期（ms）
    int m_swapIntervalMs = 50;           ///< DoubleBuffer commit 周期（ms）
    qint64 m_lastSwapTime = 0;          ///< 上次 commit 时间
};

#endif // DATAPARSETHREAD_H
