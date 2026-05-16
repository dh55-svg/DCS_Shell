#ifndef DEADBANDFILTER_H
#define DEADBANDFILTER_H
#include <cmath>

class DeadbandFilter {
public:
    explicit DeadbandFilter(float deadband) : m_deadband(deadband) {}
    void update(float newValue) { m_lastValue = newValue; }
    bool isExceeded(float newValue) const {
        return std::abs(newValue - m_lastValue) > m_deadband;
    }
    float lastValue() const { return m_lastValue; }
    void reset(float value = 0.0f) { m_lastValue = value; }
private:
    float m_deadband;
    float m_lastValue = 0.0f;
};
#endif
