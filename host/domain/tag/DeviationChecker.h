#pragma once
#include <QtMath>
#include "TagInfo.h"

struct DeviationChecker {
    static bool exceedsDeviation(float pv, float sp, float deviationLimit) {
        return qAbs(pv - sp) > deviationLimit;
    }
};
