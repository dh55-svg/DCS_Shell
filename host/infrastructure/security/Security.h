#ifndef SECURITY_H
#define SECURITY_H
#include <QString>
#include <QByteArray>
#include <QCryptographicHash>
#include <QDateTime>

/**
 * @brief 安全工具模块 — 密码哈希、输入校验、审计时间戳
 *
 * 使用 SHA-256 哈希存储密码（非明文），提供：
 *   - 密码哈希与验证（防明文泄露）
 *   - Modbus 写入值域校验（防越界写入）
 *   - 审计时间戳生成
 */
namespace Security {

/// 对密码进行 SHA-256 哈希（不存储明文）
inline QString hashPassword(const QString& password) {
    QByteArray hash = QCryptographicHash::hash(
        password.toUtf8(), QCryptographicHash::Sha256);
    return QString::fromLatin1(hash.toHex());
}

/// 验证密码是否匹配哈希
inline bool verifyPassword(const QString& password, const QString& hash) {
    return hashPassword(password) == hash;
}

/// 生成审计时间戳（ISO-8601 格式）
inline QString auditTimestamp() {
    return QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);
}

/**
 * @brief 校验 Modbus 寄存器写入值的合法性
 * @param value     写入值（uint16 范围 0-65535）
 * @param engLow    工程下限
 * @param engHigh   工程上限
 * @return 若值在工程范围内返回 true
 */
inline bool validateWriteValue(float value, float engLow, float engHigh) {
    return value >= engLow && value <= engHigh;
}

/**
 * @brief 校验 Modbus 寄存器地址的合法性
 * @param regAddr   寄存器地址
 * @param regCount  寄存器数量
 * @param maxAddr   最大允许地址（默认 65535）
 * @return 若地址在有效范围内返回 true
 */
inline bool validateRegisterAddr(int regAddr, int regCount, int maxAddr = 65535) {
    return regAddr >= 0 && regCount > 0 && (regAddr + regCount) <= maxAddr;
}

/**
 * @brief 审计日志条目结构
 */
struct AuditEntry {
    QString timestamp;   ///< ISO-8601 UTC 时间戳
    QString user;        ///< 操作员
    QString action;      ///< 操作类型 (connect/write/ack/shelve/config)
    QString target;      ///< 操作目标 (device/register/tagId)
    QString detail;      ///< 详细信息
    QString sourceIp;    ///< 来源 IP（若适用）

    QString toLogLine() const {
        return QString("[%1] %2 | %3 | %4 | %5 | %6")
            .arg(timestamp, user, action, target, detail, sourceIp);
    }
};

} // namespace Security
#endif
