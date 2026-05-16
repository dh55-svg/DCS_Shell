#ifndef ALARMEVENT_H
#define ALARMEVENT_H
#include <QString>
#include <QDateTime>
#include "../common/AlarmLimit.h"
#include "../common/DataQuality.h"
#include "../common/AlarmState.h"
/*
    报警合理化,每一条报警限都应该有一个 Rationalization 记录
    对每一个报警问 5 个问题： 1. 后果是什么？ → consequence 2. 操作员该做什么？ → operatorAction 
    3. 多久内响应？ → expectedResponseTimeSec 4. 为什么设这个限值？ → designPhilosophy 5. 谁来审批？ → approve
*/
struct AlarmRationlization{
    QString consequence;              // 报警后果描述，说明报警发生可能导致的后果
    QString operatorAction;           // 操作员应采取的行动，指导操作员如何响应报警
    int    expectedResponseTimeSec = 300; // 预期响应时间（秒），操作员应在多长时间内响应
    QString designPhilosophy;         // 设计理念，说明为什么设置这个报警
    QString approver;                 // 审批人，报警合理化文档的审批人员
    qint64 approvedDate = 0;          // 审批日期，报警合理化文档的审批时间戳
    QString correctiveAction;          // 纠正措施，报警触发后应采取的纠正行动
    QString relatedDocuments;          // 相关文档，与报警相关的文档链接或路径
    QString area;                     // 区域，报警所属的物理区域
    QString zone;                     // 分区，报警所属的逻辑分区
    int    reviewCycleMonths = 12;    // 审查周期（月），报警合理化文档的定期审查间隔
    qint64 lastReviewDate = 0;        // 最后审查日期，上次审查报警合理化文档的时间
    QString reviewer;                 // 审查人，负责审查报警合理化文档的人员
    bool   isValid = true;           // 是否有效，报警合理化文档是否当前有效

};
/*
    记录每一次报警参数修改
    sessionId / workstation — 谁在哪个工作站操作的（已预留，当前未填充）
    approved / approver / rejectReason — 审批工作流
    validUntil / autoReverted — 临时变更到期自动回退
*/
struct AlarmChangeRecord {
    qint64  changeTime = 0;           // 变更时间，报警参数变更的时间戳
    QString operatorName;             // 操作员名称，执行变更的操作员
    quint32 tagId = 0;                // 标签ID，被变更参数所属的标签
    QString fieldName;               // 变更字段名，被修改的参数字段名称
    QString oldValue;                // 旧值，参数修改前的值
    QString newValue;                // 新值，参数修改后的值
    QString reason;                  // 变更原因，说明为什么需要修改参数
    bool    approved = false;         // 是否已审批，变更是否已经获得批准
    QString approver;                // 审批人，批准变更的管理人员
    qint64  approveTime = 0;          // 审批时间，变更获得批准的时间戳
    bool    rejected = false;         // 是否被拒绝，变更是否被拒绝
    QString rejectReason;            // 拒绝原因，变更被拒绝的原因说明
    QString workOrderNo;             // 工单号，关联的工作订单编号
    qint64  validUntil = 0;          // 有效期至，临时变更的有效截止时间
    bool    autoReverted = false;     // 是否自动回退，有效期后是否自动恢复原值
    QString sessionId;               // 会话ID，操作员登录会话的唯一标识
    QString workstation;            // 工作站，执行变更的计算机名称或IP
};
/*
    报警 KPI 快照定期（每 5 分钟）生成的报警系统健康状态快照
    用处：报警性能分析报表的基础数据，可以按周/月生成趋势图，帮助工艺优化报警阈值。
*/
struct AlarmKpiSnapshot {
    qint64  timestamp = 0;           // 时间戳，KPI快照的记录时间
    int     alarmCount10min = 0;      // 10分钟内报警数量，最近10分钟产生的报警总数
    float   avgPerHour = 0.0f;       // 平均每小时报警数，报警频率的平均值
    int     peakCount10min = 0;      // 10分钟内峰值报警数，10分钟窗口内的最大报警数
    int     staleCount = 0;           // 停滞报警数量，长时间未处理的报警数量
    int     totalActive = 0;          // 活动报警总数，当前所有激活状态的报警数
    int     shelvedCount = 0;         // 搁置报警数量，被搁置的报警数量
    int     suppressedCount = 0;       // 被抑制报警数量，被抑制的报警数量
    int     floodEventCount = 0;      // 报警洪泛事件数量，报警洪泛发生的次数
    float   floodDurationMin = 0.0f;  // 洪泛持续时间（分钟），报警洪泛的平均持续时间
    float   avgAckTimeSec = 0.0f;     // 平均确认时间（秒），报警从触发到确认的平均时间
    int     chatteringCount = 0;      // 抖动报警数量，频繁触发和恢复的报警数量
    int     staleAlarmPercent = 0;      // 停滞报警百分比，停滞报警占总报警的百分比
    int     criticalCount = 0;         // 关键报警数量，Critical优先级的报警数量
    int     majorCount = 0;            // 主要报警数量，Major优先级的报警数量
    int     minorCount = 0;            // 次要报警数量，Minor优先级的报警数量
    int     advisoryCount = 0;         // 咨询报警数量，Advisory优先级的报警数量
    QStringList top5Frequent;          // 最频繁的5个报警，按触发频率排序的报警ID列表
    QStringList top5Stale;            // 最停滞的5个报警，按停滞时间排序的报警ID列表
    float   systemHealthScore = 100.0f; // 系统健康评分，系统整体健康度的量化评分（0-100）
    QString healthGrade;              // 健康等级，系统健康度的等级描述（A/B/C/D/F）
};
/*
    生命周期字段（按时间线排序）
        触发 → 确认 → 恢复 → 确认恢复 → 归档
        ① 触发	alarmId / tagId / tagName / triggerValue / thresholdValue / triggerTime / limit	
                唯一标识、谁触发了、触发了多少、什么限值类型（HH/H/L/LL）
        ② 确认	acknowledged / acknowledgeTime / acknowledgeUser	操作员在界面上点了"确认"
        ③ 恢复	returnToNormalTime / returnValue	过程值回到正常范围
        ④ 恢复确认	returnAckTime	操作员确认了恢复
        ⑤ 重复触发	firstTriggerTime / repeatCount	第一次触发时间 vs 重复次数，用于震荡检测
*/
struct AlarmEvent {
    QString   alarmId;               // 报警唯一标识符，全局唯一的报警ID
    quint32   tagId = 0;             // 关联的标签ID，触发报警的标签标识
    QString   tagName;               // 标签名称，触发报警的标签名称
    AlarmLimit     limit      = AlarmLimit::Normal; // 触发的限值类型，触发报警的限值类型
    AlarmPriority  priority   = AlarmPriority::Major; // 报警优先级，报警的紧急程度
    AlarmClassification classification = AlarmClassification::Process; // 报警分类，报警的类别
    QString   description;            // 报警描述，报警的详细说明
    float     triggerValue = 0.0f;   // 触发值，触发报警时的实际值
    float     thresholdValue = 0.0f;  // 阈值，触发报警的限值
    qint64    triggerTime = 0;        // 触发时间，报警首次触发的时间戳
    AlarmState state = AlarmState::ActiveUnack; // 报警状态，报警当前的状态
    bool      acknowledged = false;   // 是否已确认，报警是否已被操作员确认
    qint64    acknowledgeTime = 0;   // 确认时间，报警被确认的时间戳
    QString   acknowledgeUser;       // 确认用户，确认报警的操作员
    bool      active = true;         // 是否激活，报警是否处于激活状态
    qint64    returnToNormalTime = 0; // 恢复正常时间，报警恢复到正常的时间戳
    qint64    returnAckTime = 0;     // 恢复确认时间，报警恢复被确认的时间戳
    float     returnValue = 0.0f;    // 恢复值，报警恢复时的实际值
    bool      shelved = false;       // 是否已搁置，报警是否被暂时搁置
    qint64    shelvedTime = 0;       // 搁置时间，报警被搁置的时间戳
    QString   shelveReason;          // 搁置原因，搁置报警的原因说明
    int       shelveDurationSec = 0; // 搁置持续时间（秒），报警搁置的有效时长
    QString   shelveUser;           // 搁置用户，执行搁置操作的操作员
    AlarmSuppressionType suppressionType = AlarmSuppressionType::None; // 抑制类型，报警被抑制的类型
    QString   suppressionReason;     // 抑制原因，报警被抑制的原因说明
    QString   suppressionUser;       // 抑制用户，执行抑制操作的操作员
    qint64    suppressionTime = 0;   // 抑制时间，报警被抑制的时间戳
    bool      outOfService = false;  // 是否退出服务，报警点是否已退出服务
    QString   outOfServiceReason;   // 退出服务原因，报警点退出服务的原因
    QString   outOfServiceUser;     // 退出服务用户，执行退出服务操作的操作员
    QString   workOrderNo;         // 工单号，关联的工单编号
    QString   operatorAnnotation;   // 操作员注释，操作员添加的备注信息
    qint64    annotationTime = 0;    // 注释时间，注释添加的时间戳
    QString   annotationUser;       // 注释用户，添加注释的操作员
    QString   area;                // 区域，报警所属的物理区域
    QString   zone;                // 分区，报警所属的逻辑分区
    AlarmNotificationType notificationType = AlarmNotificationType::Audible; // 通知类型，报警通知的方式
    qint64    lastNotificationTime = 0; // 最后通知时间，上次发送通知的时间戳
    int       notificationCount = 0;   // 通知次数，已发送通知的总次数
    int       repeatCount = 0;        // 重复次数，报警重复触发的次数
    qint64    firstTriggerTime = 0;    // 首次触发时间，报警首次触发的时间戳
    // ActiveUnack || Activeack — 报警正在活跃
    bool isActive() const {
        return state == AlarmState::ActiveUnack
               || state == AlarmState::Activeack;
    }
    // 同上，需要操作员关注
    bool needsAttention() const {
        return state == AlarmState::ActiveUnack
               || state == AlarmState::Activeack;
    }
    // SuppressedByDesign || outOfService
    bool isSuppressed() const {
        return state == AlarmState::SuppressedByDesign
               || state == AlarmState::outOfService;
    }
    // Shelved
    bool isShelved() const {
        return state == AlarmState::Shelved;
    }
};
//查询报警列表的过滤条件
struct AlarmFilter {
    QList<AlarmPriority> priorities;      // 优先级过滤，指定要查询的报警优先级列表
    QList<AlarmClassification> classifications; // 分类过滤，指定要查询的报警分类列表
    QList<AlarmState> states;            // 状态过滤，指定要查询的报警状态列表
    QStringList areas;                   // 区域过滤，指定要查询的区域列表
    qint64     fromTime = 0;             // 起始时间，查询的起始时间戳
    qint64     toTime = 0;               // 结束时间，查询的结束时间戳
    QString    keyword;                  // 关键词，搜索报警的关键词
    bool       includeShelved = false;   // 是否包含搁置报警，查询结果是否包含搁置的报警
    bool       includeSuppressed = false; // 是否包含被抑制报警，查询结果是否包含被抑制的报警
    bool       includeOutOfService = false; // 是否包含退出服务报警，查询结果是否包含退出服务的报警
};
//条件抑制规则的定义
struct SuppressionRule {
    quint32     ruleId = 0;              // 规则ID，抑制规则的唯一标识符
    quint32     targetTagId;            // ★ 被抑制的目标位号（如 FIC_101）
    quint32     conditionTagId;          // 条件位号（如 PUMP_101 运转信号）
    QString     conditionExpr;          // 条件表达式（如 "value == 0"）
    QString     reason;                 // 抑制原因，设置抑制规则的原因说明
    bool        enabled = true;         // 是否启用，抑制规则是否当前启用
    QString     createdBy;              // 创建人，创建抑制规则的操作员
    QString     approver;              // 审批人，批准抑制规则的管理人员
    qint64      createdTime = 0;        // 创建时间，抑制规则创建的时间戳
};
//搁置操作的历史记录
struct ShelveRecord {
    quint32 tagId = 0;           // 位号 ID
    qint64  shelveTime = 0;      // 搁置开始时间
    qint64  unshelveTime = 0;    // 搁置结束时间（0=仍搁置中）
    int     durationSec = 0;     // 搁置时长
    QString reason;              // 搁置原因
    QString user;                // 操作员（空=系统自动）
    bool    isAuto = false;      // 是否系统自动搁置
};
//报警洪泛事件记录
struct AlarmFloodEvent {
    qint64     startTime = 0;           // 开始时间，报警洪泛开始的时间戳
    qint64     endTime = 0;              // 结束时间，报警洪泛结束的时间戳
    int        alarmCount = 0;           // 报警数量，洪泛期间产生的报警总数
    int        peakRate = 0;            // 峰值速率，洪泛期间的最大报警速率（报警/分钟）
    int        weightedCount = 0;       // 加权报警计数（Critical=5, Major=3, Minor/Advisory=1）
    QStringList topContributors;        // 主要贡献者，触发最多报警的标签ID列表
    QString    triggerCause;            // 触发原因，导致报警洪泛的原因分析
    QString    analyst;                // 分析人员，负责分析洪泛事件的人员
    qint64     analysisTime = 0;        // 分析时间，完成洪泛分析的时间戳
};
#endif // ALARMEVENT_H
