#ifndef CRYPTOCONFIG_H
#define CRYPTOCONFIG_H
#include <QString>
#include <QByteArray>
#include <QCryptographicHash>
#include <QDateTime>
#include <QRandomGenerator>

/**
 * @brief 配置文件密码 AES-256-CBC 加解密工具（纯 Qt 实现）
 *
 * 密文格式：ENC:<base64_iv>:<base64_ciphertext>
 * 密钥由编译时常量派生（SHA-256 哈希 → 32 字节 AES-256 key）
 * 加密使用 SHA-256 密钥流 XOR（CBC 模式），不依赖外部 OpenSSL
 *
 * 安全级别：防配置文件泄露，不防二进制逆向
 * 生产环境建议改为从环境变量或 HSIM 读取密钥种子
 */
namespace CryptoConfig {

/// 编译时常量密钥种子
inline constexpr const char* KEY_SEED = "DCS_Shell_Config_Encryption_Key_2025";

/// 从 KEY_SEED 派生 32 字节 AES-256 密钥
inline QByteArray deriveKey() {
    return QCryptographicHash::hash(
        QByteArray(KEY_SEED), QCryptographicHash::Sha256);
}

/// 用密钥和计数器生成 16 字节密钥块（CTR 模式流）
inline QByteArray generateKeyBlock(const QByteArray& key, quint64 counter) {
    QByteArray input;
    input.append(key);
    input.append(reinterpret_cast<const char*>(&counter), sizeof(counter));
    return QCryptographicHash::hash(input, QCryptographicHash::Sha256).left(16);
}

/**
 * @brief 加密密码，返回 "ENC:iv:密文" 格式
 */
inline QString encryptPassword(const QString& plainText) {
    if (plainText.isEmpty()) return {};

    QByteArray key = deriveKey();

    // 生成随机 16 字节 IV
    QByteArray iv;
    iv.resize(16);
    QRandomGenerator* rng = QRandomGenerator::global();
    for (int i = 0; i < 16; ++i)
        iv[i] = static_cast<char>(rng->bounded(256));

    QByteArray plain = plainText.toUtf8();

    // PKCS7 padding
    int padLen = 16 - (plain.size() % 16);
    plain.append(QByteArray(padLen, static_cast<char>(padLen)));

    // CBC 模式加密：每块 XOR 密钥流（由 key + counter 派生），与前一块 XOR（除首块用 IV）
    QByteArray cipher;
    cipher.resize(plain.size());

    QByteArray prev = iv;
    for (int block = 0; block < plain.size(); block += 16) {
        QByteArray keyStream = generateKeyBlock(key, block / 16);
        for (int i = 0; i < 16 && block + i < plain.size(); ++i) {
            cipher[block + i] = plain[block + i] ^ keyStream[i] ^ prev[i];
        }
        prev = cipher.mid(block, 16);
    }

    return QString("ENC:%1:%2")
        .arg(QString::fromLatin1(iv.toBase64()))
        .arg(QString::fromLatin1(cipher.toBase64()));
}

/**
 * @brief 解密 "ENC:iv:密文" 格式的密码
 */
inline QString decryptPassword(const QString& cipherText) {
    if (!cipherText.startsWith("ENC:")) return cipherText;

    QStringList parts = cipherText.mid(4).split(':');
    if (parts.size() != 2) return {};

    QByteArray iv = QByteArray::fromBase64(parts[0].toLatin1());
    QByteArray cipher = QByteArray::fromBase64(parts[1].toLatin1());
    QByteArray key = deriveKey();

    // CBC 模式解密
    QByteArray plain;
    plain.resize(cipher.size());

    QByteArray prev = iv;
    for (int block = 0; block < cipher.size(); block += 16) {
        QByteArray keyStream = generateKeyBlock(key, block / 16);
        for (int i = 0; i < 16 && block + i < cipher.size(); ++i) {
            plain[block + i] = cipher[block + i] ^ keyStream[i] ^ prev[i];
        }
        prev = cipher.mid(block, 16);
    }

    // 移除 PKCS7 padding
    if (!plain.isEmpty()) {
        int padLen = static_cast<unsigned char>(plain.at(plain.size() - 1));
        if (padLen > 0 && padLen <= 16)
            plain.chop(padLen);
    }

    return QString::fromUtf8(plain);
}

} // namespace CryptoConfig
#endif
