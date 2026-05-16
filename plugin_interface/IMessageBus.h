#ifndef IMESSAGEBUS_H
#define IMESSAGEBUS_H
#include <cstdint>

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
    virtual void push(const RawModbusData& data) = 0;
    virtual bool pop(RawModbusData& out) = 0;
    virtual size_t available() const = 0;
    virtual void clear() = 0;
};
#endif
