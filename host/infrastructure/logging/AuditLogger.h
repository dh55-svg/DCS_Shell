#ifndef AUDITLOGGER_H
#define AUDITLOGGER_H
#include <QString>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QMutex>
#include <QMutexLocker>
#include "../security/Security.h"

/**
 * @brief 操作审计日志 — 记录所有工业控制关键操作
 *
 * 架构角色：横切关注点，注入到所有写路径（DataPipeline、AlarmEngine、PluginHub）
 *
 * 默认写入 logs/audit_YYYYMMDD.log，线程安全，独立于操作日志
 * 满足 NERC CIP-007 / IEC 62443-3-3 的可审计性要求
 */
class AuditLogger {
public:
    AuditLogger() = default;
    ~AuditLogger() {
        QMutexLocker lock(&m_mutex);
        if (m_file.isOpen()) m_file.close();
    }

    /// 设置审计日志目录（首次调用 mkpath 创建）
    void setLogDir(const QString& dir) {
        QMutexLocker lock(&m_mutex);
        m_dir = dir;
    }

    /**
     * @brief 记录一条审计条目
     * @param user     操作员标识
     * @param action   操作类型（connect/write/ack/shelve/unshelve/suppress/switch/config）
     * @param target   操作目标（device/register/tagId/pluginIid）
     * @param detail   详细信息（值/原因/来源）
     */
    void record(const QString& user, const QString& action,
                const QString& target, const QString& detail) {
        QMutexLocker lock(&m_mutex);

        Security::AuditEntry entry;
        entry.timestamp = Security::auditTimestamp();
        entry.user = user.isEmpty() ? "system" : user;
        entry.action = action;
        entry.target = target;
        entry.detail = detail;

        openFileIfNeeded();
        if (m_file.isOpen()) {
            QTextStream stream(&m_file);
            stream << entry.toLogLine() << "\n";
            stream.flush();
        }
    }

    /// 操作员登录记录
    void recordLogin(const QString& user, const QString& source) {
        record(user, "login", "system", source.isEmpty() ? "local" : source);
    }

    /// 操作员登出记录
    void recordLogout(const QString& user) {
        record(user, "logout", "system", "");
    }

private:
    void openFileIfNeeded() {
        QString today = QDateTime::currentDateTime().toString("yyyyMMdd");
        QString expectedName = m_dir + "/audit_" + today + ".log";

        if (m_currentFileName == expectedName) return;

        if (m_file.isOpen()) m_file.close();

        QDir().mkpath(m_dir);
        m_file.setFileName(expectedName);
        m_file.open(QIODevice::Append | QIODevice::Text);
        m_currentFileName = expectedName;
    }

    QMutex m_mutex;
    QString m_dir = "./logs";
    QFile m_file;
    QString m_currentFileName;
};

#endif
