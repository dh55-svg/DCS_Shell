#ifndef ALARMSTATEMACHINE_H
#define ALARMSTATEMACHINE_H
#include "AlarmEvent.h"

class AlarmStateMachine {
public:
    static bool canTransition(AlarmState from, AlarmState to) {
        switch (from) {
        case AlarmState::Normal:
            return to == AlarmState::ActiveUnack || to == AlarmState::outOfService;
        case AlarmState::ActiveUnack:
            return to == AlarmState::Activeack || to == AlarmState::ReturnToNormalunack
                || to == AlarmState::Shelved || to == AlarmState::SuppressedByDesign;
        case AlarmState::Activeack:
            return to == AlarmState::ReturnToNormalunack || to == AlarmState::Shelved
                || to == AlarmState::SuppressedByDesign || to == AlarmState::ActiveUnack;
        case AlarmState::ReturnToNormalunack:
            return to == AlarmState::ReturnToNormalack || to == AlarmState::ActiveUnack;
        case AlarmState::ReturnToNormalack:
            return to == AlarmState::Normal || to == AlarmState::ActiveUnack;
        case AlarmState::Shelved:
            return to == AlarmState::ActiveUnack || to == AlarmState::ReturnToNormalunack;
        case AlarmState::SuppressedByDesign:
            return to == AlarmState::ActiveUnack || to == AlarmState::ReturnToNormalunack
                   || to == AlarmState::outOfService;
        case AlarmState::outOfService:
            return to == AlarmState::ActiveUnack || to == AlarmState::ReturnToNormalunack;
        default: return false;
        }
    }
};
#endif
