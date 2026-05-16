#include <QtTest>
#include <QElapsedTimer>
#include <QDebug>
#include <QRandomGenerator>
#include <QDateTime>
#include "domain/alarm/FloodDetector.h"
#include "infrastructure/messaging/DoubleBuffer.h"
#include "infrastructure/messaging/LockFreeRingBuffer.h"

/**
 * @file    test_performance.cpp
 * @brief   性能/负载测试 — 大点位吞吐、报警洪水、内存长期稳定性
 *
 * 三个测试用例集成到 ctest，每个在 30 秒内完成。
 * 依赖项均来自 host_core 或 header-only，无需链接 DCS_Shell。
 */

class TestPerformance : public QObject {
    Q_OBJECT

private slots:

    // ── 测试 1：10000 位号 DoubleBuffer 批量写入吞吐量 ──
    void large_point_throughput() {
        DoubleBuffer doubleBuf;

        QElapsedTimer timer;
        timer.start();

        // 批量写入 10000 个位号快照
        for (quint32 i = 0; i < 10000; ++i) {
            DoubleBuffer::Snapshot snap;
            snap.tagId = i;
            snap.currentValue = static_cast<float>(i % 100);
            snap.timestamp = QDateTime::currentMSecsSinceEpoch();
            snap.quality = DataQuality::Good;
            doubleBuf.write(snap.tagId, snap);
        }
        doubleBuf.commit();

        qint64 elapsed = timer.elapsed();
        QVERIFY(elapsed < 200);  // 10000 次写入 + commit 应在 200ms 内

        // 验证可读
        auto snap = doubleBuf.readAll();
        QVERIFY(snap != nullptr);
        QCOMPARE(snap->size(), static_cast<size_t>(10000));

        qDebug() << "[perf] 10000 tags written + committed in" << elapsed << "ms";
    }

    // ── 测试 2：报警洪水压力测试 ──
    void alarm_flood_stress() {
        FloodDetector detector;

        QElapsedTimer timer;
        timer.start();

        // 快速注入 500 个报警，验证 FloodDetector 不会崩溃
        for (int i = 0; i < 500; ++i) {
            detector.recordAlarm(
                static_cast<quint32>(i % 100),
                QString("TAG_%1").arg(i % 100),
                (i % 10 == 0) ? AlarmPriority::Critical : AlarmPriority::Major);
        }

        qint64 elapsed = timer.elapsed();
        QVERIFY(elapsed < 1000);  // 500 报警应在 1 秒内
        QVERIFY(detector.isInFlood());  // 超过阈值 10，应处于洪水状态
        qDebug() << "[perf] 500 alarms injected in" << elapsed << "ms, flood active:" << detector.isInFlood();
    }

    // ── 测试 3：DoubleBuffer 长期运行内存稳定性 ──
    void memory_stability() {
        DoubleBuffer doubleBuf;
        auto* rng = QRandomGenerator::global();

        // 加速模拟：100 轮 × 100 位号 commit
        constexpr int TOTAL_ROUNDS = 100;
        constexpr int TAGS_PER_ROUND = 100;

        for (int round = 0; round < TOTAL_ROUNDS; ++round) {
            for (quint32 i = 0; i < TAGS_PER_ROUND; ++i) {
                DoubleBuffer::Snapshot snap;
                snap.tagId = i;
                snap.currentValue = static_cast<float>(rng->bounded(10000)) / 100.0f;
                snap.timestamp = QDateTime::currentMSecsSinceEpoch();
                doubleBuf.write(snap.tagId, snap);
            }
            doubleBuf.commit();

            // 每 20 轮验证可读
            if (round % 20 == 0) {
                auto snap = doubleBuf.readAll();
                QVERIFY(snap != nullptr);
            }
        }

        // 最终一致性
        auto snap = doubleBuf.readAll();
        QVERIFY(snap != nullptr);
        QCOMPARE(snap->size(), static_cast<size_t>(TAGS_PER_ROUND));
        qDebug() << "[perf] memory stability: 100 rounds x 100 tags, final size:" << snap->size();
    }
};

QTEST_APPLESS_MAIN(TestPerformance)
#include "test_performance.moc"
