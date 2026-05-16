#include <QtTest>
#include "infrastructure/messaging/DoubleBuffer.h"

class TestDoubleBuffer : public QObject {
    Q_OBJECT
private slots:
    void write_and_read_single() {
        DoubleBuffer db;
        DoubleBuffer::Snapshot snap;
        snap.tagId = 101;
        snap.currentValue = 42.5f;
        snap.quality = DataQuality::Good;
        db.write(101, snap);
        db.commit();
        auto result = db.readTag(101);
        QCOMPARE(result.tagId, 101u);
        QVERIFY(qFuzzyCompare(result.currentValue, 42.5f));
    }
    void read_missing_tag_returns_empty() {
        DoubleBuffer db;
        auto result = db.readTag(999);
        QCOMPARE(result.tagId, 0u);
    }
    void batch_write_and_readAll() {
        DoubleBuffer db;
        std::vector<DoubleBuffer::Snapshot> snaps(3);
        for (int i = 0; i < 3; ++i) {
            snaps[i].tagId = 100 + i;
            snaps[i].currentValue = float(i * 10);
        }
        db.writeBatch(snaps);
        db.commit();
        auto all = db.readAll();
        QCOMPARE(all->size(), 3ul);
    }
    void overwrite_then_read_latest() {
        DoubleBuffer db;
        DoubleBuffer::Snapshot s1{101, 100.0f};
        db.write(101, s1);
        db.commit();
        DoubleBuffer::Snapshot s2{101, 200.0f};
        db.write(101, s2);
        db.commit();
        QVERIFY(qFuzzyCompare(db.readTag(101).currentValue, 200.0f));
    }
    void multiple_commits_reuse_pool() {
        DoubleBuffer db;
        for (int i = 0; i < 20; ++i) {
            DoubleBuffer::Snapshot s{static_cast<quint32>(i), float(i)};
            db.write(static_cast<quint32>(i), s);
            db.commit();
        }
        QVERIFY(qFuzzyCompare(db.readTag(19).currentValue, 19.0f));
    }
};

QTEST_APPLESS_MAIN(TestDoubleBuffer)
#include "test_double_buffer.moc"
