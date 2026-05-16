#ifndef IMESSAGEBUS_H
#define IMESSAGEBUS_H
#include <cstdint>
#include <vector>

struct RawModbusData {
    int deviceId = 0;
    int serverAddr = 1;
    int regAddr = 0;
    uint16_t value = 0;
    int64_t timestamp = 0;
};

class IMessageBus {
public:
    virtual ~IMessageBus() = default;

    // ── 基础入队/出队 ──
    virtual bool enqueue(const RawModbusData& data) = 0;
    virtual bool dequeue(RawModbusData& out) = 0;

    // ── 批量 ──
    virtual size_t dequeueBatch(std::vector<RawModbusData>& output, size_t maxCount) = 0;

    // ── 状态查询 ──
    virtual bool empty() const = 0;
    virtual size_t size() const = 0;

    // ── 兼容旧 API ──
    void push(const RawModbusData& data) { enqueue(data); }
    bool pop(RawModbusData& out) { return dequeue(out); }
    size_t available() const { return size(); }
    void clear() { RawModbusData dummy; while (dequeue(dummy)) {} }
};
#endif
