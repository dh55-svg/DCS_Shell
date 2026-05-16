#ifndef ALARMSTATE_H
#define ALARMSTATE_H
#include <QtTypes>
enum class AlarmState : qint8 {
    Normal = 0,
    ActiveUnack,
    Activeack,
    ReturnToNormalunack,
    ReturnToNormalack,
    Shelved,
    SuppressedByDesign,
    outOfService
};
enum class AlarmSuppressionType : quint8 {
    None = 0,
    DesignSuppression,
    OutOfService,
    Interlock,
    Override
};
enum class AlarmPriority : quint8 {
    Advisory = 0,
    Minor,
    Major,
    Critical
};
enum class AlarmClassification : quint8 {
    Process = 0,
    Safety,
    Environmental,
    Quality,
    Machinery,
    Electrical,
    Instrument
};
enum class AlarmNotificationType : quint8 {
    None = 0,
    Visual,
    Audible,
    Page,
    Email,
    Escalation
};
enum class TagType : quint8 {
    AI = 0,
    AO,
    DI,
    DO,
    PID
};
#endif
