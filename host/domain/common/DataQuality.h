#ifndef DATAQUALITY_H
#define DATAQUALITY_H
#include <cstdint>
enum class DataQuality : uint8_t {
    Good = 0,
    Uncertain = 1,
    Bad = 2,
    Stale = 3,
    InitialValue = 4,
    ManualEntry = 5
};
#endif
