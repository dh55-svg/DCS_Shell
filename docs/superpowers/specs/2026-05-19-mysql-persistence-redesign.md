# MysqlPersistencePlugin 重设计规格说明

**日期:** 2026-05-19
**状态:** 已批准
**范围:** 将 MysqlPersistencePlugin 从空壳 stub 提升为商业标准实现

---

## 1. 背景与动机

MysqlPersistencePlugin 实现了 5 个接口共 24 个方法，但其中 19 个是空实现（stub），且存在以下问题：

- `m_configured` 死字段：写而不读
- `batchInsert` 无事务：逐条 INSERT，性能差
- stub 方法静默吞掉调用：`insert`/`update`/`remove` 返回 `true`，调用方误以为成功
- SQL 无错误处理：`q.exec()` 返回值未检查
- 连接名硬编码：多实例冲突

## 2. 设计决策

### 2.1 实现范围

通过代码搜索确认，24 个方法中只有 5 个在业务层实际被调用：

| 接口 | 方法 | 调用位置 |
|------|------|---------|
| IAlarmRepo | `insertEvent` | AlarmEngine.cpp:240 |
| IAlarmRepo | `updateAck` | AlarmEngine.cpp:366 |
| IAlarmRepo | `insertKpiSnapshot` | AlarmEngine.cpp:699 |
| IHistoryRepo | `batchInsert` | HistorySampler.cpp:87 |
| IHistoryRepo | `query` | HistorySampler.cpp:148 |

**决策：** 只实现这 5 个方法，其余 19 个保持 stub 但加 `qWarning` 日志。

### 2.2 不实现的接口

- **ITagRepo (7 方法)**：TagManager 自己读写 JSON，绕过了 ITagRepo 接口，零调用
- **IOperationRepo (2 方法)**：零调用，且接口设计返回 QJsonObject 不适合 MySQL 强类型

### 2.3 Stub 策略

| 返回类型 | 处理 | 原因 |
|----------|------|------|
| void | 打 warning，空实现 | 不影响调用方 |
| 返回容器 | 打 warning，return {} | 空结果安全 |
| 返回 bool | 打 warning，return false | 原来 return true 是 bug |
| 返回对象 | 打 warning，return 默认值 | TagInf{} 安全 |

## 3. 数据库设计

### 3.1 表结构

**alarm_events** — 报警事件表：

```sql
CREATE TABLE alarm_events (
    alarm_id          VARCHAR(64) PRIMARY KEY,
    tag_id            INT NOT NULL,
    tag_name          VARCHAR(128),
    description       TEXT,
    limit_type        TINYINT DEFAULT 0,
    priority          TINYINT DEFAULT 2,
    classification    TINYINT DEFAULT 0,
    state             TINYINT DEFAULT 1,
    trigger_value     FLOAT DEFAULT 0,
    threshold_value   FLOAT DEFAULT 0,
    trigger_time      BIGINT DEFAULT 0,
    acknowledged      TINYINT DEFAULT 0,
    ack_time          BIGINT DEFAULT 0,
    ack_user          VARCHAR(64),
    active            TINYINT DEFAULT 1,
    area              VARCHAR(64),
    zone              VARCHAR(64),
    created_at        TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_alarm_state (state, active),
    INDEX idx_alarm_time (trigger_time)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
```

**alarm_kpi_snapshots** — KPI 快照表：

```sql
CREATE TABLE alarm_kpi_snapshots (
    id                BIGINT AUTO_INCREMENT PRIMARY KEY,
    timestamp         BIGINT NOT NULL,
    alarm_count_10min INT DEFAULT 0,
    avg_per_hour      FLOAT DEFAULT 0,
    peak_count_10min  INT DEFAULT 0,
    stale_count       INT DEFAULT 0,
    total_active      INT DEFAULT 0,
    shelved_count     INT DEFAULT 0,
    suppressed_count  INT DEFAULT 0,
    critical_count    INT DEFAULT 0,
    major_count       INT DEFAULT 0,
    minor_count       INT DEFAULT 0,
    advisory_count    INT DEFAULT 0,
    avg_ack_time_sec  FLOAT DEFAULT 0,
    chattering_count  INT DEFAULT 0,
    system_health_score FLOAT DEFAULT 100,
    health_grade      VARCHAR(2),
    INDEX idx_kpi_ts (timestamp)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
```

**history_records** — 历史数据表：

```sql
CREATE TABLE history_records (
    id                BIGINT AUTO_INCREMENT PRIMARY KEY,
    tag_id            INT NOT NULL,
    value             DOUBLE DEFAULT 0,
    quality           INT DEFAULT 0,
    timestamp         BIGINT NOT NULL,
    INDEX idx_history_tag_time (tag_id, timestamp)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
```

**operation_logs** — 操作日志表：

```sql
CREATE TABLE operation_logs (
    id                BIGINT AUTO_INCREMENT PRIMARY KEY,
    user_name         VARCHAR(64),
    action            VARCHAR(128),
    target            VARCHAR(256),
    detail            TEXT,
    timestamp         BIGINT,
    INDEX idx_op_time (timestamp)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
```

### 3.2 索引策略

- `alarm_events`: state+active 复合索引（查询活跃报警）、trigger_time 索引（时间范围查询）
- `alarm_kpi_snapshots`: timestamp 索引（时间序列查询）
- `history_records`: tag_id+timestamp 复合索引（按标签查历史，覆盖主查询场景）
- `operation_logs`: timestamp 索引（时间范围查询）

## 4. 核心方法实现

### 4.1 insertEvent

使用 `INSERT ... ON DUPLICATE KEY UPDATE`，重复 alarmId 则更新状态字段：

```sql
INSERT INTO alarm_events (...) VALUES (...)
ON DUPLICATE KEY UPDATE state=VALUES(state), acknowledged=VALUES(acknowledged),
ack_time=VALUES(ack_time), ack_user=VALUES(ack_user), active=VALUES(active)
```

### 4.2 updateAck

```sql
UPDATE alarm_events SET acknowledged=1, ack_time=?, ack_user=? WHERE alarm_id=?
```

### 4.3 insertKpiSnapshot

```sql
INSERT INTO alarm_kpi_snapshots (...) VALUES (...)
```

追加写入，不做去重。

### 4.4 batchInsert

事务包裹，统一提交：

```
m_db.transaction()
  for each record:
    INSERT INTO history_records (tag_id, value, quality, timestamp) VALUES (?,?,?,?)
    if failed → rollback, return
m_db.commit()
```

### 4.5 query

```sql
SELECT tag_id, value, quality, timestamp FROM history_records
WHERE tag_id=? AND timestamp>=? AND timestamp<=?
ORDER BY timestamp LIMIT ?
```

返回 `QVector<HistoryRecord>`。

## 5. 连接管理

### 5.1 连接名

使用 `QUuid::createUuid()` 生成唯一连接名，避免多实例冲突。

### 5.2 连接选项

```
MYSQL_OPT_RECONNECT=1     -- 断线自动重连
CLIENT_INTERACTIVE=1      -- 使用交互式超时
```

### 5.3 析构

```cpp
m_db.close();
QSqlDatabase::removeDatabase(m_connName);
```

## 6. 错误处理

### 6.1 统一辅助方法

```cpp
bool MysqlPersistencePlugin::execQuery(QSqlQuery& q) const {
    if (!q.exec()) {
        qWarning() << "[MySQL] SQL error:" << q.lastError().text()
                    << "| query:" << q.lastQuery();
        return false;
    }
    return true;
}
```

### 6.2 错误策略

- 所有写操作：失败时打 `qWarning`，不抛异常
- 查询操作：失败时返回空容器
- 连接失败：`qCritical` 级别日志
- 事务失败：自动 rollback

## 7. 头文件变更

### 新增成员

```cpp
private:
    QSqlDatabase m_db;
    QString m_connName;         // 替代硬编码连接名
    void initDb();
    bool execQuery(QSqlQuery& q) const;
    AlarmEvent toAlarmEvent(QSqlQuery& q) const;
    AlarmChangeRecord toChangeRecord(QSqlQuery& q) const;
    AlarmKpiSnapshot toKpiSnapshot(QSqlQuery& q) const;
    TagInf toTagInf(QSqlQuery& q) const;
```

### 删除

- `bool m_configured = false;` — 死字段，实际靠 `m_db.isOpen()` 判断

## 8. 枚举映射

所有枚举存储为 TINYINT，使用 `static_cast<int>()` 转换：

| 枚举 | 存储类型 | 值范围 |
|------|---------|--------|
| AlarmLimit | TINYINT | 0-6 |
| AlarmState | TINYINT | 0-7 |
| AlarmPriority | TINYINT | 0-3 |
| AlarmClassification | TINYINT | 0-6 |
| AlarmSuppressionType | TINYINT | 0-4 |
| AlarmNotificationType | TINYINT | 0-5 |
| TagType | TINYINT | 0-4 |
| DataQuality | INT | 0-5 |

## 9. 不做的事情

- 不实现 ITagRepo（TagManager 绕过接口直接读 JSON）
- 不实现 IOperationRepo（零调用）
- 不实现未被调用的 query 方法（queryActive、queryEvents 等）
- 不做连接池（当前单连接够用，未来可扩展）
- 不做 ORM 映射层（直接 SQL，够轻量）
