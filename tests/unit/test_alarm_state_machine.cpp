#include <QtTest>
#include "domain/alarm/AlarmStateMachine.h"

class TestAlarmStateMachine : public QObject {
    Q_OBJECT
private slots:
    void normal_to_activeUnack() {
        QVERIFY(AlarmStateMachine::canTransition(AlarmState::Normal, AlarmState::ActiveUnack));
    }
    void normal_to_outOfService() {
        QVERIFY(AlarmStateMachine::canTransition(AlarmState::Normal, AlarmState::outOfService));
    }
    void normal_rejects_ack() {
        QVERIFY(!AlarmStateMachine::canTransition(AlarmState::Normal, AlarmState::Activeack));
    }
    void activeUnack_to_ack() {
        QVERIFY(AlarmStateMachine::canTransition(AlarmState::ActiveUnack, AlarmState::Activeack));
    }
    void activeUnack_to_shelved() {
        QVERIFY(AlarmStateMachine::canTransition(AlarmState::ActiveUnack, AlarmState::Shelved));
    }
    void activeUnack_to_suppressed() {
        QVERIFY(AlarmStateMachine::canTransition(AlarmState::ActiveUnack, AlarmState::SuppressedByDesign));
    }
    void activeUnack_to_rtn_unack() {
        QVERIFY(AlarmStateMachine::canTransition(AlarmState::ActiveUnack, AlarmState::ReturnToNormalunack));
    }
    void shelved_to_activeUnack() {
        QVERIFY(AlarmStateMachine::canTransition(AlarmState::Shelved, AlarmState::ActiveUnack));
    }
    void outOfService_to_activeUnack() {
        QVERIFY(AlarmStateMachine::canTransition(AlarmState::outOfService, AlarmState::ActiveUnack));
    }
    void returnToNormalack_to_normal() {
        QVERIFY(AlarmStateMachine::canTransition(AlarmState::ReturnToNormalack, AlarmState::Normal));
    }
    void invalid_from_normal_to_shelved() {
        QVERIFY(!AlarmStateMachine::canTransition(AlarmState::Normal, AlarmState::Shelved));
    }
    void full_state_coverage() {
        // Verify all 8 states can be checked without crash
        AlarmState states[] = {AlarmState::Normal, AlarmState::ActiveUnack, AlarmState::Activeack,
                               AlarmState::ReturnToNormalunack, AlarmState::ReturnToNormalack,
                               AlarmState::Shelved, AlarmState::SuppressedByDesign, AlarmState::outOfService};
        for (auto from : states)
            for (auto to : states)
                AlarmStateMachine::canTransition(from, to); // must not crash
    }
};

QTEST_MAIN(TestAlarmStateMachine)
#include "test_alarm_state_machine.moc"
