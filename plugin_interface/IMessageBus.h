#ifndef IMESSAGEBUS_H
#define IMESSAGEBUS_H
#include <cstdint>
#include <vector>

struct RawModbusData {
    int deviceId = 0;
    int serverAddr = 1;
    int startAddr = 0;
    int count = 1;
    uint16_t values[256] = {};
    int64_t timestamp = 0;
};

class IMessageBus {
public:
    virtual ~IMessageBus() = default;
    virtual bool enqueue(const RawModbusData& data) = 0;
    virtual bool dequeue(RawModbusData& out) = 0;
    virtual size_t dequeueBatch(std::vector<RawModbusData>& output, size_t maxCount) = 0;
    virtual bool empty() const = 0;
    virtual size_t size() const = 0;
    void push(const RawModbusData& data) { enqueue(data); }
    bool pop(RawModbusData& out) { return dequeue(out); }
    size_t available() const { return size(); }
    void clear() { RawModbusData dummy; while (dequeue(dummy)) {} }
};
#endif
