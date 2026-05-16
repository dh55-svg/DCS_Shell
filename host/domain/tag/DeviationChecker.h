#ifndef DEVIATIONCHECKER_H
#define DEVIATIONCHECKER_H
#include <cmath>

class DeviationChecker {
public:
    explicit DeviationChecker(float limit) : m_limit(limit) {}
    bool check(float pv, float sp) const {
        return std::abs(pv - sp) > m_limit;
    }
    float limit() const { return m_limit; }
private:
    float m_limit;
};
#endif
