#include <QtTest>
#include "pipeline/DataPipeline.h"
#include "mocks/mock_fieldbus.h"
#include "mocks/mock_history_repo.h"
#include "domain/tag/tagmanager.h"
#include "infrastructure/nulls/NullTagRepo.h"
#include "infrastructure/nulls/NullAlarmRepo.h"
#include "domain/alarm/AlarmEngine.h"

class TestPipelineWithSimulator : public QObject {
    Q_OBJECT
private slots:
    void pipeline_start_stop_lifecycle() {
        MockFieldbus fieldbus;
        NullTagRepo tagRepo;
        TagManager tagMgr(tagRepo, nullptr);
        NullAlarmRepo alarmRepo;
        AlarmEngine alarmEngine(alarmRepo, &tagMgr, nullptr);
        alarmEngine.initialize();
        MockHistoryRepo historyRepo;

        DataPipeline pipeline;
        pipeline.setFieldbus(&fieldbus);
        pipeline.setTagManager(&tagMgr);
        pipeline.setAlarmEngine(&alarmEngine);
        pipeline.setHistoryRepo(&historyRepo);

        // Start should not crash
        pipeline.start();
        QVERIFY(fieldbus.isRunning());

        // Stop should not crash
        pipeline.stop();
        QVERIFY(!fieldbus.isRunning());
    }
    void double_buffer_accessible() {
        DataPipeline pipeline;
        QVERIFY(pipeline.doubleBuffer() != nullptr);
    }
};

QTEST_MAIN(TestPipelineWithSimulator)
#include "test_pipeline_with_simulator.moc"
