#pragma once
#include "../common/AlarmLimit.h"

struct DeadbandFilter {
    static bool exceedsDeadbaud(float value, float threshold, float deadband, AlarmLimit limit, float prevValue) {
        switch (limit) {
        case AlarmLimit::HighHigh:
        case AlarmLimit::High:
            return value > threshold + deadband || (value > threshold && prevValue <= threshold);
        case AlarmLimit::Low:
        case AlarmLimit::LowLow:
            return value < threshold - deadband || (value < threshold && prevValue >= threshold);
        default: return false;
        }
    }
    static bool returnsToNormal(float value, float threshold, float deadband, AlarmLimit limit) {
        switch (limit) {
        case AlarmLimit::HighHigh:
        case AlarmLimit::High:
            return value <= threshold - deadband;
        case AlarmLimit::Low:
        case AlarmLimit::LowLow:
            return value >= threshold + deadband;
        default: return true;
        }
    }
};
