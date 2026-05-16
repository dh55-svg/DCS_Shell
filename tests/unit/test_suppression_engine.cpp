#include <QtTest>
#include "domain/alarm/SuppressionEngine.h"
#include "infrastructure/messaging/DoubleBuffer.h"

class TestSuppressionEngine : public QObject {
    Q_OBJECT
private slots:
    void add_rule_succeeds() {
        SuppressionEngine engine;
        SuppressionRule rule;
        rule.ruleId = 1; rule.targetTagId = 101; rule.enabled = true;
        QVERIFY(engine.addRule(rule));
        QCOMPARE(engine.rules().size(), 1);
    }
    void duplicate_rule_id_rejected() {
        SuppressionEngine engine;
        SuppressionRule r1; r1.ruleId = 1; r1.targetTagId = 101;
        SuppressionRule r2; r2.ruleId = 1; r2.targetTagId = 102;
        engine.addRule(r1);
        QVERIFY(!engine.addRule(r2));
    }
    void remove_rule() {
        SuppressionEngine engine;
        SuppressionRule r; r.ruleId = 1; r.targetTagId = 101;
        engine.addRule(r);
        engine.removeRule(1);
        QVERIFY(engine.rules().isEmpty());
    }
    void disabled_rule_does_not_evaluate() {
        SuppressionEngine engine;
        SuppressionRule r; r.ruleId = 1; r.targetTagId = 101; r.enabled = false;
        engine.addRule(r);
        QVERIFY(!engine.evaluate(101));
    }
    void enabled_rule_evaluates_true() {
        SuppressionEngine engine;
        SuppressionRule r; r.ruleId = 1; r.targetTagId = 101; r.enabled = true;
        engine.addRule(r);
        QVERIFY(engine.evaluate(101));
    }
    void conditional_rule_without_datasource_evaluates_true() {
        SuppressionEngine engine;
        SuppressionRule r; r.ruleId = 1; r.targetTagId = 101; r.enabled = true;
        r.conditionTagId = 200; r.conditionExpr = "value == 0";
        engine.addRule(r);
        // No datasource set → condition can't be checked → defaults to suppress
        QVERIFY(engine.evaluate(101));
    }
    void condition_expression_parsing() {
        SuppressionEngine engine;
        DoubleBuffer db;
        DoubleBuffer::Snapshot s{200, 0.0f};
        db.write(200, s); db.commit();
        engine.setDataSource(&db);

        SuppressionRule r; r.ruleId = 1; r.targetTagId = 101; r.enabled = true;
        r.conditionTagId = 200; r.conditionExpr = "value == 0";
        engine.addRule(r);
        QVERIFY(engine.evaluate(101));
    }
};

QTEST_MAIN(TestSuppressionEngine)
#include "test_suppression_engine.moc"
