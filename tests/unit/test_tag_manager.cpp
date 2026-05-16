#include <QtTest>
#include "domain/tag/tagmanager.h"
#include "infrastructure/nulls/NullTagRepo.h"

class TestTagManager : public QObject {
    Q_OBJECT
private:
    NullTagRepo* m_repo = nullptr;
    TagManager* m_mgr = nullptr;

private slots:
    void init() {
        m_repo = new NullTagRepo();
        m_mgr = new TagManager(*m_repo, nullptr);
    }
    void cleanup() {
        delete m_mgr;
        delete m_repo;
    }

    void add_tag_increases_count() {
        TagInf tag;
        tag.tagId = 101; tag.tagName = "TIC_101";
        QVERIFY(m_mgr->addTag(tag));
        QCOMPARE(m_mgr->tagCount(), 1);
    }
    void get_tag_by_id() {
        TagInf tag; tag.tagId = 101; tag.tagName = "TIC_101";
        m_mgr->addTag(tag);
        QCOMPARE(m_mgr->getTag(101).tagName, "TIC_101");
    }
    void get_tag_by_name() {
        TagInf tag; tag.tagId = 102; tag.tagName = "PIC_102";
        m_mgr->addTag(tag);
        QCOMPARE(m_mgr->getTagByName("PIC_102").tagId, 102u);
    }
    void remove_tag_decreases_count() {
        TagInf tag; tag.tagId = 101; tag.tagName = "TIC_101";
        m_mgr->addTag(tag);
        m_mgr->removeTag(101);
        QCOMPARE(m_mgr->tagCount(), 0);
    }
    void range_query() {
        TagInf tag; tag.tagId = 101; tag.engHigh = 200.0f; tag.engLow = 0.0f;
        m_mgr->addTag(tag);
        auto range = m_mgr->getRange(101);
        QVERIFY(qFuzzyCompare(range.first, 0.0f));
        QVERIFY(qFuzzyCompare(range.second, 200.0f));
    }
    void alarm_limits_query() {
        TagInf tag; tag.tagId = 101;
        tag.highHighLimit = 180; tag.highLimit = 150; tag.lowLimit = 20;
        tag.lowLowLimit = 5; tag.deadband = 3;
        m_mgr->addTag(tag);
        auto limits = m_mgr->getAlarmLimits(101);
        QVERIFY(qFuzzyCompare(limits.highHigh, 180));
        QVERIFY(qFuzzyCompare(limits.deadband, 3));
    }
    void modbus_mapping() {
        TagInf tag; tag.tagId = 101;
        tag.modbusServerAddr = 1; tag.modbusRegAddr = 10; tag.modbusRegCount = 2;
        m_mgr->addTag(tag);
        auto map = m_mgr->getModbusMapping(101);
        QCOMPARE(map.serverAddr, 1);
        QCOMPARE(map.regAddr, 10);
        QCOMPARE(map.regCount, 2);
    }
    void find_by_modbus_addr() {
        TagInf tag; tag.tagId = 101;
        tag.modbusServerAddr = 1; tag.modbusRegAddr = 10;
        m_mgr->addTag(tag);
        QCOMPARE(m_mgr->findTagByModbusAddr(1, 10), 101u);
    }
    void duplicate_tag_id_rejected() {
        TagInf t1; t1.tagId = 101; t1.tagName = "A";
        TagInf t2; t2.tagId = 101; t2.tagName = "B";
        QVERIFY(m_mgr->addTag(t1));
        QVERIFY(!m_mgr->addTag(t2));
    }
    void clear_removes_all() {
        for (int i = 0; i < 3; ++i) {
            TagInf t; t.tagId = 100 + i; t.tagName = QString("TAG_%1").arg(i);
            m_mgr->addTag(t);
        }
        m_mgr->clear();
        QCOMPARE(m_mgr->tagCount(), 0);
    }
    void get_tags_by_device() {
        TagInf t; t.tagId = 0x01000001; t.tagName = "DEV1_TAG";
        t.modbusServerAddr = 1;
        m_mgr->addTag(t);
        auto tags = m_mgr->getTagsByDevice(1);
        QVERIFY(!tags.isEmpty());
    }
};

QTEST_APPLESS_MAIN(TestTagManager)
#include "test_tag_manager.moc"
