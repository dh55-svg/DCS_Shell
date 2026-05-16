#ifndef RATEOFCHANGECHECKER_H
#define RATEOFCHANGECHECKER_H
#include <deque>
#include <cmath>
#include <cstdint>

struct RocSample {
    float value;
    int64_t timestampMs;
};

class RateOfChangeChecker {
public:
    RateOfChangeChecker(float limitPerSec, int windowMs)
        : m_limitPerSec(limitPerSec), m_windowMs(windowMs) {}

    void addSample(float value, int64_t timestampMs) {
        m_samples.push_back({value, timestampMs});
        prune(timestampMs);
    }

    bool isExceeded() const {
        if (m_samples.size() < 2) return false;
        float dv = std::abs(m_samples.back().value - m_samples.front().value);
        float dt = (m_samples.back().timestampMs - m_samples.front().timestampMs) / 1000.0f;
        if (dt <= 0.0f) return false;
        return (dv / dt) > m_limitPerSec;
    }

    void reset() { m_samples.clear(); }

private:
    void prune(int64_t nowMs) {
        while (!m_samples.empty() && (nowMs - m_samples.front().timestampMs) > m_windowMs)
            m_samples.pop_front();
    }

    float m_limitPerSec;
    int m_windowMs;
    std::deque<RocSample> m_samples;
};
#endif
