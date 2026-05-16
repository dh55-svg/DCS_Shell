#ifndef ALARMLIMIT_H
#define ALARMLIMIT_H
#include <QtTypes>
enum class AlarmLimit : qint8 {
    Normal       = 0,
    LowLow       = 1,
    Low          = 2,
    High         = 3,
    HighHigh     = 4,
    Deviation    = 5,
    RateOfChange = 6
};
#endif
