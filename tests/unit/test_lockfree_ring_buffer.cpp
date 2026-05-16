#include <QtTest>
#include "infrastructure/messaging/LockFreeRingBuffer.h"

class TestLockFreeRingBuffer : public QObject {
    Q_OBJECT
private slots:
    void push_pop_single() {
        LockFreeRingBuffer<int, 64> buf;
        QVERIFY(buf.enqueue(42));
        int out;
        QVERIFY(buf.dequeue(out));
        QCOMPARE(out, 42);
    }
    void empty_pop_returns_false() {
        LockFreeRingBuffer<int, 64> buf;
        int out;
        QVERIFY(!buf.dequeue(out));
    }
    void full_push_returns_false() {
        LockFreeRingBuffer<int, 4> buf;
        for (int i = 0; i < 4; ++i) buf.enqueue(i);
        QVERIFY(!buf.enqueue(99));
    }
    void size_tracks_elements() {
        LockFreeRingBuffer<int, 64> buf;
        buf.enqueue(1); buf.enqueue(2);
        QCOMPARE(buf.size(), 2ul);
        int out; buf.dequeue(out);
        QCOMPARE(buf.size(), 1ul);
    }
    void empty_after_all_dequeued() {
        LockFreeRingBuffer<int, 64> buf;
        buf.enqueue(1); buf.enqueue(2);
        int out; buf.dequeue(out); buf.dequeue(out);
        QVERIFY(buf.empty());
    }
    void batch_dequeue() {
        LockFreeRingBuffer<int, 64> buf;
        for (int i = 0; i < 10; ++i) buf.enqueue(i);
        std::vector<int> result;
        buf.dequeueBatch(result, 5);
        QCOMPARE(result.size(), 5ul);
        QCOMPARE(buf.size(), 5ul);
    }
    void ring_buffer_wraps_correctly() {
        LockFreeRingBuffer<int, 4> buf;
        int out;
        for (int round = 0; round < 3; ++round) {
            for (int i = 0; i < 4; ++i) buf.enqueue(i);
            for (int i = 0; i < 4; ++i) { buf.dequeue(out); QCOMPARE(out, i); }
        }
        QVERIFY(buf.empty());
    }
};

QTEST_MAIN(TestLockFreeRingBuffer)
#include "test_lockfree_ring_buffer.moc"
