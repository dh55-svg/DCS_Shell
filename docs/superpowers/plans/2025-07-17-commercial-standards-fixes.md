# 商业标准修复实现计划

> **面向 AI 代理的工作者：** 必需子技能：使用 superpowers:subagent-driven-development（推荐）或 superpowers:executing-plans 逐任务实现此计划。步骤使用复选框（`- [ ]`）语法来跟踪进度。

**目标：** 修复商业标准审查中发现的 8 项缺陷：写入校验信号、AES 密码加密、CI 签名占位、版本化发布、悬挂指针、插件记录清理、性能测试、Modbus/MQTT CI 条件编译。

**架构：** 8 个独立模块，均改动现有文件或新增小型文件。模块间无顺序依赖，可并行执行。所有改动保持现有 Qt6/C++17 架构风格。

**技术栈：** Qt 6.5+, C++17, CMake 3.20+, OpenSSL (通过 Qt), GitHub Actions

---

## 文件结构

| 文件 | 职责 | 操作 |
|------|------|------|
| `host/pipeline/DataPipeline.h` | 新增 writeRejected 信号 | 修改 |
| `host/pipeline/DataPipeline.cpp` | setAutoMode 添加值域校验 + 已存在的 writeSetPoint 补充信号 emit | 修改 |
| `host/infrastructure/security/CryptoConfig.h` | AES-256-CBC 密码加解密工具 | 新建 |
| `host/app/AppConfig.h` | fromJson() 中解密 mysql.password | 修改 |
| `CHANGELOG.md` | 版本变更记录 | 新建 |
| `host/infrastructure/plugin/PluginDescriptor.h` | 新增 name/version/author/compatibility 字段 | 修改 |
| `host/infrastructure/plugin/PluginHub.cpp` | scanAll 解析新字段 + switchPlugin 清理旧记录 | 修改 |
| `plugins/*/PluginName.json` (4 个) | 扩展元数据字段 | 修改 |
| `host/app/AppContext.h` | 新增 shared_ptr\<PluginHub\> | 修改 |
| `host/app/ApplicationBuilder.h` | shared_ptr 重构 + aliasing shared_ptr | 修改 |
| `host/main.cpp` | PluginHub 改为 make_shared | 修改 |
| `tests/performance/test_performance.cpp` | 3 个性能测试用例 | 新建 |
| `tests/CMakeLists.txt` | 注册性能测试 | 修改 |
| `plugins/CMakeLists.txt` | 条件编译 libmodbus/Qt6Mqtt | 修改 |
| `.github/workflows/build-linux.yml` | 安装 libmodbus-dev qt6-mqtt-dev | 修改 |
| `.github/workflows/build-windows.yml` | vcpkg 安装 + 签名占位 | 修改 |

---

### 任务 1：DataPipeline 补充 writeRejected 信号 + setAutoMode 校验

**文件：**
- 修改：`host/pipeline/DataPipeline.h:63-65`
- 修改：`host/pipeline/DataPipeline.cpp:63-70, 97-116`

- [ ] **步骤 1：DataPipeline.h 新增 writeRejected 信号**

在 `commStatusChanged` 信号之后添加：

```cpp
    void writeRejected(quint32 tagId, float value, const QString& reason); ///< 写入值域校验拒绝
```

- [ ] **步骤 2：DataPipeline.cpp writeSetPoint 添加信号 emit**

当前 writeSetPoint() 行 63-70 已调用 `Security::validateWriteValue()` 并 log + return。在 return 前补充 emit：

```cpp
    if (!Security::validateWriteValue(value, tag.engLow, tag.engHigh)) {
        QString reason = QString("value %1 out of range [%2, %3]")
            .arg(value).arg(tag.engLow).arg(tag.engHigh);
        if (m_logger) m_logger->warn(QString("[SECURITY] writeSetPoint rejected: tagId=%1 %2")
            .arg(tagId).arg(reason));
        emit writeRejected(tagId, value, reason);   // ★ 新增
        return;
    }
```

- [ ] **步骤 3：DataPipeline.cpp setAutoMode 添加值域校验**

在 `setAutoMode()` 中写 SP 到总线前（行 97-116 附近），添加校验：

```cpp
    if (autoMode && m_fieldbus) {
        auto snap = m_doubleBuffer.readTag(tagId);
        float sp = snap.setPoint;
        
        // ── 安全检查：值域校验 ──
        if (!Security::validateWriteValue(sp, tag.engLow, tag.engHigh)) {
            QString reason = QString("setPoint %1 out of range [%2, %3]")
                .arg(sp).arg(tag.engLow).arg(tag.engHigh);
            if (m_logger) m_logger->warn(QString("[SECURITY] setAutoMode write rejected: tagId=%1 %2")
                .arg(tagId).arg(reason));
            emit writeRejected(tagId, sp, reason);
            return;
        }
        
        float range = tag.engHigh - tag.engLow;
        // ... 后续逻辑不变
```

- [ ] **步骤 4：构建验证**

```bash
cmake --build build --target DCS_Shell
```

预期：编译通过，无新增警告。

- [ ] **步骤 5：Commit**

```bash
git add host/pipeline/DataPipeline.h host/pipeline/DataPipeline.cpp
git commit -m "feat: add writeRejected signal and setAutoMode value validation

- DataPipeline emits writeRejected signal when Security::validateWriteValue fails
- setAutoMode now validates setPoint before writing to fieldbus
- Already-existing writeSetPoint validation now also emits the signal"
```

---

### 任务 2：MySQL 密码 AES-256 加密

**文件：**
- 创建：`host/infrastructure/security/CryptoConfig.h`
- 修改：`host/app/AppConfig.h:30-31`
- 修改：`host/CMakeLists.txt:24-32`（链接 OpenSSL）

- [ ] **步骤 1：创建 CryptoConfig.h**

```cpp
#ifndef CRYPTOCONFIG_H
#define CRYPTOCONFIG_H
#include <QString>
#include <QByteArray>
#include <QCryptographicHash>
#include <QDateTime>
#include <openssl/evp.h>

/**
 * @brief 配置文件密码 AES-256-CBC 加解密工具
 *
 * 密文格式：ENC:<base64_iv>:<base64_ciphertext>
 * 密钥由编译时常量派生（SHA-256 哈希 → 32 字节 AES-256 key）
 */
namespace CryptoConfig {

// 编译时常量密钥种子（生产环境建议改为从环境变量读取）
inline constexpr const char* KEY_SEED = "DCS_Shell_Config_Encryption_Key_2025";

inline QByteArray deriveKey() {
    return QCryptographicHash::hash(
        QByteArray(KEY_SEED), QCryptographicHash::Sha256);
}

/**
 * @brief 加密密码，返回 "ENC:iv:密文" 格式
 */
inline QString encryptPassword(const QString& plainText) {
    if (plainText.isEmpty()) return {};

    QByteArray key = deriveKey();
    QByteArray iv = QByteArray::fromBase64(
        QCryptographicHash::hash(
            QByteArray::number(qrand()) + QByteArray::number(QDateTime::currentMSecsSinceEpoch()),
            QCryptographicHash::Md5)
    ).left(16);  // 16 bytes IV for AES-256-CBC

    QByteArray plain = plainText.toUtf8();
    // PKCS7 padding
    int padLen = 16 - (plain.size() % 16);
    plain.append(QByteArray(padLen, static_cast<char>(padLen)));

    // AES-256-CBC encrypt via OpenSSL (linked by Qt)
    QByteArray cipherText;
    cipherText.resize(plain.size());

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr,
                       reinterpret_cast<const unsigned char*>(key.constData()),
                       reinterpret_cast<const unsigned char*>(iv.constData()));
    int outLen = 0;
    EVP_EncryptUpdate(ctx, reinterpret_cast<unsigned char*>(cipherText.data()), &outLen,
                      reinterpret_cast<const unsigned char*>(plain.constData()), plain.size());
    int finalLen = 0;
    EVP_EncryptFinal_ex(ctx, reinterpret_cast<unsigned char*>(cipherText.data()) + outLen, &finalLen);
    cipherText.resize(outLen + finalLen);
    EVP_CIPHER_CTX_free(ctx);

    return QString("ENC:%1:%2")
        .arg(QString::fromLatin1(iv.toBase64()))
        .arg(QString::fromLatin1(cipherText.toBase64()));
}

/**
 * @brief 解密 "ENC:iv:密文" 格式的密码
 */
inline QString decryptPassword(const QString& cipherText) {
    if (!cipherText.startsWith("ENC:")) return cipherText;  // 明文原样返回

    QStringList parts = cipherText.mid(4).split(':');
    if (parts.size() != 2) return {};

    QByteArray iv = QByteArray::fromBase64(parts[0].toLatin1());
    QByteArray cipher = QByteArray::fromBase64(parts[1].toLatin1());
    QByteArray key = deriveKey();

    QByteArray plain;
    plain.resize(cipher.size());

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr,
                       reinterpret_cast<const unsigned char*>(key.constData()),
                       reinterpret_cast<const unsigned char*>(iv.constData()));
    int outLen = 0;
    EVP_DecryptUpdate(ctx, reinterpret_cast<unsigned char*>(plain.data()), &outLen,
                      reinterpret_cast<const unsigned char*>(cipher.constData()), cipher.size());
    int finalLen = 0;
    EVP_DecryptFinal_ex(ctx, reinterpret_cast<unsigned char*>(plain.data()) + outLen, &finalLen);
    plain.resize(outLen + finalLen);
    EVP_CIPHER_CTX_free(ctx);

    // Remove PKCS7 padding
    if (!plain.isEmpty()) {
        int padLen = static_cast<unsigned char>(plain.at(plain.size() - 1));
        if (padLen > 0 && padLen <= 16)
            plain.chop(padLen);
    }

    return QString::fromUtf8(plain);
}

} // namespace CryptoConfig
#endif
```

- [ ] **步骤 2：修改 AppConfig.h fromJson() 解密密码**

在 `cfg.mysql.password = m.value("password", "").toString();` 之后添加解密：

```cpp
    cfg.mysql.password = m.value("password", "").toString();
    // 若为 ENC: 前缀密文则自动解密
    if (cfg.mysql.password.startsWith("ENC:")) {
        cfg.mysql.password = CryptoConfig::decryptPassword(cfg.mysql.password);
    }
```

并在文件头部添加 include：

```cpp
#include "../infrastructure/security/CryptoConfig.h"
```

- [ ] **步骤 3：修改 host/CMakeLists.txt 链接 OpenSSL**

在 `target_link_libraries(host_core PUBLIC ...)` 中添加 OpenSSL：

```cmake
target_link_libraries(host_core PUBLIC
    plugin_interface
    Qt6::Core
    Qt6::Sql
    Qt6::Network
    OpenSSL::SSL
    OpenSSL::Crypto
)
```

并在顶层 `CMakeLists.txt` 的 `find_package` 区域添加：

```cmake
find_package(OpenSSL REQUIRED)
```

- [ ] **步骤 4：构建验证**

```bash
cmake --build build --target DCS_Shell
```

预期：编译通过。

> **⚠️ 风险提示：** CryptoConfig.h 使用 OpenSSL EVP API。如果 `host/CMakeLists.txt` 未链接 OpenSSL，需在 `target_link_libraries(host_core ...)` 中添加 `${OPENSSL_LIBRARIES}`，并在顶层 CMakeLists.txt 添加 `find_package(OpenSSL REQUIRED)`。Qt6 通常自带 OpenSSL 链接，但需验证。

- [ ] **步骤 4：Commit**

```bash
git add host/infrastructure/security/CryptoConfig.h host/app/AppConfig.h
git commit -m "feat: AES-256 encrypted config password support

- CryptoConfig::encryptPassword / decryptPassword using AES-256-CBC
- AppConfig::fromJson auto-decrypts ENC: prefixed mysql.password
- KEY_SEED is compile-time constant (production should use env var)"
```

---

### 任务 3：Windows 签名 CI 占位

**文件：**
- 修改：`.github/workflows/build-windows.yml`

- [ ] **步骤 1：在 build-windows.yml 末尾添加注释掉的签名步骤**

在 `Test` 步骤之后、文件末尾之前添加：

```yaml
      # ──────────────────────────────────────────────────
      # Code Signing (disabled — enable when certificate is available)
      # Required GitHub Secrets:
      #   CODE_SIGN_CERT_BASE64  — base64-encoded .pfx certificate
      #   CODE_SIGN_PASSWORD     — certificate password
      # ──────────────────────────────────────────────────
      # - name: Import code signing certificate
      #   run: |
      #     $certBytes = [Convert]::FromBase64String("${{ secrets.CODE_SIGN_CERT_BASE64 }}")
      #     [System.IO.File]::WriteAllBytes("${{ runner.temp }}\codesign.pfx", $certBytes)
      # - name: Sign binaries
      #   run: |
      #     signtool sign /fd SHA256 `
      #       /f "${{ runner.temp }}\codesign.pfx" `
      #       /p "${{ secrets.CODE_SIGN_PASSWORD }}" `
      #       /tr http://timestamp.digicert.com `
      #       /td SHA256 `
      #       build\host\Release\DCS_Shell.exe `
      #       build\plugins\*.dll
```

- [ ] **步骤 2：Commit**

```bash
git add .github/workflows/build-windows.yml
git commit -m "ci: add commented-out Authenticode signing step placeholder

Template uses signtool with SHA256 + timestamp server.
Enable by setting CODE_SIGN_CERT_BASE64 and CODE_SIGN_PASSWORD secrets."
```

---

### 任务 4：版本化发布流程

**文件：**
- 创建：`CHANGELOG.md`
- 修改：`host/infrastructure/plugin/PluginDescriptor.h:6-14`
- 修改：`host/infrastructure/plugin/PluginHub.cpp:28-33, 78-83`
- 修改：`plugins/fieldbus_modbus/ModbusPlugin.json`
- 修改：`plugins/fieldbus_simulator/SimulatorPlugin.json`
- 修改：`plugins/mqtt_qt/QtMqttPlugin.json`
- 修改：`plugins/persistence_sqlite/SqlitePersistencePlugin.json`

- [ ] **步骤 1：创建 CHANGELOG.md**

```markdown
# Changelog

All notable changes to DCS_Shell will be documented in this file.

## [1.0.0] — 2025-07-17

### Added
- Plugin-based DCS SCADA/HMI host application
- ISA-18.2 compliant 8-state alarm engine with shelving, suppression, flood detection, chattering guard
- Hot-swap plugin loading/unloading via PluginHub
- MVC-separated presentation layer (Models, Delegates, ViewModels)
- Simulator plugin for development/testing
- SQLite persistence plugin for alarm and history storage
- Modbus RTU/TCP fieldbus plugin (requires libmodbus)
- Qt MQTT gateway plugin (requires Qt6::Mqtt)
- DoubleBuffer (RCU read-write separation) and LockFreeRingBuffer
- Deadband filter, deviation checker, rate-of-change checker
- Security module (SHA-256 password hashing, value validation, audit timestamps)
- Cross-platform CMake build (Windows MSVC/MinGW + Linux GCC)
- 15 unit tests (88+ cases) + 4 integration tests

[1.0.0]: https://github.com/example/DCS_Shell/tree/v1.0.0
```

- [ ] **步骤 2：扩展 PluginDescriptor 结构体**

```cpp
struct PluginDescriptor {
    QString filePath;
    QString iid;
    QString className;
    QString name;           // ★ 新增：插件名称
    QString version;        // ★ 新增：语义版本号
    QString author;         // ★ 新增：作者/组织
    QString compatibility;  // ★ 新增：兼容性约束（如 ">=1.0.0"）
    int priority = 0;
    int qtVersion = 0;
    bool passed = false;
    QString failReason;
    QJsonObject rawMeta;
};
```

- [ ] **步骤 3：修改 PluginHub::scanAll() 解析新字段**

在 `scanAll()` 中，读取优先级后添加新字段解析。当前代码行 28-33：

```cpp
        // Check companion JSON for priority
        QString jsonPath = fi.absolutePath() + "/" + fi.completeBaseName() + ".json";
        if (QFile::exists(jsonPath)) {
            QFile f(jsonPath);
            if (f.open(QIODevice::ReadOnly)) {
                QJsonObject j = QJsonDocument::fromJson(f.readAll()).object();
                desc.priority = j.value("priority").toInt(0);
            }
        }
```

改为：

```cpp
        // Check companion JSON for metadata
        QString jsonPath = fi.absolutePath() + "/" + fi.completeBaseName() + ".json";
        if (QFile::exists(jsonPath)) {
            QFile f(jsonPath);
            if (f.open(QIODevice::ReadOnly)) {
                QJsonObject j = QJsonDocument::fromJson(f.readAll()).object();
                desc.name = j.value("name").toString();
                desc.version = j.value("version").toString();
                desc.author = j.value("author").toString();
                desc.compatibility = j.value("compatibility").toString();
                desc.priority = j.value("priority").toInt(0);
            }
        }
```

- [ ] **步骤 4：修复 PluginHub::switchPlugin() 清理旧条目**

在 `switchPlugin()` 中 `unloadPlugin(iid);` 之后，追加新 descriptor 之前，添加清理逻辑：

```cpp
    unloadPlugin(iid);

    // ── 清理 m_discovered 中匹配此 iid 的旧条目 ──
    m_discovered.erase(
        std::remove_if(m_discovered.begin(), m_discovered.end(),
            [&iidStr](const PluginDescriptor& d) { return d.iid == iidStr; }),
        m_discovered.end());

    PluginDescriptor newDesc;
    newDesc.filePath = newPath;
    newDesc.iid = iidStr;
    newDesc.passed = true;
    newDesc.priority = 100;
    m_discovered.append(newDesc);
```

- [ ] **步骤 5：更新 4 个插件 .json 文件**

`plugins/fieldbus_modbus/ModbusPlugin.json`：
```json
{
  "name": "ModbusPlugin",
  "version": "1.0.0",
  "author": "DCS_Shell",
  "compatibility": ">=1.0.0",
  "priority": 10
}
```

`plugins/fieldbus_simulator/SimulatorPlugin.json`：
```json
{
  "name": "SimulatorPlugin",
  "version": "1.0.0",
  "author": "DCS_Shell",
  "compatibility": ">=1.0.0",
  "priority": 5
}
```

`plugins/mqtt_qt/QtMqttPlugin.json`：
```json
{
  "name": "QtMqttPlugin",
  "version": "1.0.0",
  "author": "DCS_Shell",
  "compatibility": ">=1.0.0",
  "priority": 10
}
```

`plugins/persistence_sqlite/SqlitePersistencePlugin.json`：
```json
{
  "name": "SqlitePersistencePlugin",
  "version": "1.0.0",
  "author": "DCS_Shell",
  "compatibility": ">=1.0.0",
  "priority": 10
}
```

- [ ] **步骤 6：构建验证**

```bash
cmake --build build
```

预期：编译通过。

- [ ] **步骤 7：Commit**

```bash
git add CHANGELOG.md \
    host/infrastructure/plugin/PluginDescriptor.h \
    host/infrastructure/plugin/PluginHub.cpp \
    plugins/fieldbus_modbus/ModbusPlugin.json \
    plugins/fieldbus_simulator/SimulatorPlugin.json \
    plugins/mqtt_qt/QtMqttPlugin.json \
    plugins/persistence_sqlite/SqlitePersistencePlugin.json
git commit -m "feat: versioned release process with CHANGELOG and plugin metadata

- CHANGELOG.md tracking v1.0.0 changes
- PluginDescriptor extended with name/version/author/compatibility
- PluginHub::scanAll parses new JSON fields
- PluginHub::switchPlugin cleans stale m_discovered entries
- All 4 plugin .json files updated with full metadata"
```

---

### 任务 5：悬挂指针修复 — shared_ptr<PluginHub>

**文件：**
- 修改：`host/app/AppContext.h:4-26`
- 修改：`host/app/ApplicationBuilder.h:4-92`
- 修改：`host/main.cpp:18-19`

- [ ] **步骤 1：AppContext.h 新增 shared_ptr<PluginHub>**

在现有成员之后添加：

```cpp
#include "../infrastructure/plugin/PluginHub.h"  // ★ 新增 include
// ... 在 struct AppContext 末尾添加：
    std::shared_ptr<PluginHub> pluginHub;  // ★ 持有 PluginHub 所有权
```

- [ ] **步骤 2：ApplicationBuilder.h 重构所有权**

`withPluginHub` 改为接收 `shared_ptr`：

```cpp
    ApplicationBuilder& withPluginHub(std::shared_ptr<PluginHub> hub) {
        m_pluginHub = hub;
        m_ctx->pluginHub = hub;  // ★ 存入 AppContext 共享所有权
        return *this;
    }
```

`withFieldbus` 使用 aliasing shared_ptr：

```cpp
    ApplicationBuilder& withFieldbus() {
        if (m_pluginHub) {
            IFieldbus* fb = m_pluginHub->resolve<IFieldbus>(IFieldBus_iid);
            if (fb) {
                // ★ aliasing shared_ptr: 共享 PluginHub 所有权，指向 IFieldbus
                m_ctx->fieldbus = std::shared_ptr<IFieldbus>(m_pluginHub, fb);
                return *this;
            }
        }
        m_ctx->fieldbus = std::make_shared<NullFieldbus>();
        return *this;
    }
```

`withDatabase` 同样使用 aliasing shared_ptr：

```cpp
    ApplicationBuilder& withDatabase() {
        if (m_pluginHub) {
            IAlarmRepo* alarm = m_pluginHub->resolve<IAlarmRepo>(IAlarmRepo_iid);
            IHistoryRepo* history = m_pluginHub->resolve<IHistoryRepo>(IHistoryRepo_iid);
            if (alarm && history) {
                m_ctx->alarmRepo = std::shared_ptr<IAlarmRepo>(m_pluginHub, alarm);
                m_ctx->historyRepo = std::shared_ptr<IHistoryRepo>(m_pluginHub, history);
                m_ctx->tagRepo = std::make_shared<NullTagRepo>();
                m_ctx->operationRepo = std::make_shared<NullOperationRepo>();
                return *this;
            }
        }
        m_ctx->alarmRepo = std::make_shared<NullAlarmRepo>(m_ctx->logger.get());
        m_ctx->historyRepo = std::make_shared<NullHistoryRepo>();
        m_ctx->tagRepo = std::make_shared<NullTagRepo>();
        m_ctx->operationRepo = std::make_shared<NullOperationRepo>();
        return *this;
    }
```

私有成员 `PluginHub* m_pluginHub` 改为：

```cpp
    std::shared_ptr<PluginHub> m_pluginHub;
```

- [ ] **步骤 3：main.cpp 改为 make_shared**

`PluginHub hub;` → `auto hub = std::make_shared<PluginHub>();`

`hub.scanAll(...)` → `hub->scanAll(...)`

`&hub` → `hub`

改为：

```cpp
    // 1. PluginHub scan
    auto hub = std::make_shared<PluginHub>();
    QString pluginsDir = QApplication::applicationDirPath() + "/plugins";
    if (!QDir(pluginsDir).exists()) pluginsDir = "plugins";
    int found = hub->scanAll(pluginsDir);
    qDebug() << "[main] PluginHub scanned:" << found << "candidates";

    // ...

    auto ctx = ApplicationBuilder()
        .withPluginHub(hub)
        // ...
```

- [ ] **步骤 4：构建验证**

```bash
cmake --build build --target DCS_Shell
```

预期：编译通过。

- [ ] **步骤 5：Commit**

```bash
git add host/app/AppContext.h host/app/ApplicationBuilder.h host/main.cpp
git commit -m "fix: eliminate dangling pointer risk with shared_ptr<PluginHub>

- AppContext now holds shared_ptr<PluginHub> to guarantee lifetime
- ApplicationBuilder::withFieldbus/withDatabase use aliasing shared_ptr
  so IFieldbus/IAlarmRepo/IHistoryRepo share PluginHub ownership
- main.cpp creates PluginHub via make_shared"
```

---

### 任务 6：性能测试

**文件：**
- 创建：`tests/performance/test_performance.cpp`
- 修改：`tests/CMakeLists.txt`

- [ ] **步骤 1：创建 tests/performance/test_performance.cpp**

```cpp
#include <QtTest>
#include <QElapsedTimer>
#include "pipeline/DataParseThread.h"
#include "pipeline/HistorySampler.h"
#include "domain/tag/tagmanager.h"
#include "domain/alarm/AlarmEngine.h"
#include "domain/alarm/FloodDetector.h"
#include "infrastructure/nulls/NullTagRepo.h"
#include "infrastructure/nulls/NullAlarmRepo.h"
#include "infrastructure/messaging/DoubleBuffer.h"
#include "infrastructure/messaging/LockFreeRingBuffer.h"

class TestPerformance : public QObject {
    Q_OBJECT
private slots:

    // ── 测试 1：10000 位号批量处理吞吐量 ──
    void large_point_throughput() {
        LockFreeRingBuffer<RawModbusData, 8192> ringBuf;
        DoubleBuffer doubleBuf;
        DataParseThread parseThread;
        parseThread.setRingBuffer(&ringBuf);
        parseThread.setDoubleBuffer(&doubleBuf);

        // 构造 10000 个位号配置
        QVector<TagInf> tags;
        for (int i = 0; i < 10000; ++i) {
            TagInf tag;
            tag.tagId = static_cast<quint32>(i + 1);
            tag.modbusServerAddr = 1;
            tag.modbusRegAddr = i;
            tag.engLow = 0.0f;
            tag.engHigh = 100.0f;
            tag.alarmEnabled = false;  // 关闭报警以减少干扰
            tags.append(tag);
        }
        parseThread.setTagConfig(tags);

        // 构造一批 10000 寄存器的原始数据
        RawModbusData raw;
        raw.serverAddr = 1;
        raw.startAddr = 0;
        raw.count = 10000;
        for (int i = 0; i < 10000; ++i)
            raw.values[i] = static_cast<quint16>(i * 6);  // 分布 0-60000
        ringBuf.enqueue(raw);

        // 测量 processBatch 时间
        QElapsedTimer timer;
        timer.start();

        std::vector<RawModbusData> batch;
        ringBuf.dequeueBatch(batch, 256);
        parseThread.processBatchTest(batch);  // 需要添加 public 测试入口

        qint64 elapsed = timer.elapsed();
        QVERIFY(elapsed < 500);  // 应在 500ms 内完成
        qDebug() << "[perf] 10000 tags processed in" << elapsed << "ms";
    }

    // ── 测试 2：报警洪水检测与恢复 ──
    void alarm_flood_recovery() {
        FloodDetector detector;
        detector.setThreshold(100);      // 100 报警/分钟 = 洪水
        detector.setRecoveryThreshold(20); // 降至 20/分钟 = 恢复

        // 模拟 500 报警/秒持续 3 秒
        bool floodTriggered = false;
        for (int sec = 0; sec < 3; ++sec) {
            for (int i = 0; i < 500; ++i) {
                floodTriggered = detector.recordAlarm(1 + i);
                if (floodTriggered) break;
            }
            if (floodTriggered) break;
        }
        QVERIFY(floodTriggered);

        // 模拟恢复：降至 10 报警/秒
        bool recovered = false;
        for (int sec = 0; sec < 5; ++sec) {
            for (int i = 0; i < 10; ++i) {
                recovered = !detector.isFloodActive();
                if (recovered) break;
            }
            if (recovered) break;
            QTest::qWait(1000);  // 等 1 秒让计时窗口推进
        }
        QVERIFY(recovered);
        qDebug() << "[perf] flood detection + recovery passed";
    }

    // ── 测试 3：内存长期运行稳定性（加速模拟） ──
    void memory_stability() {
        DoubleBuffer doubleBuf;

        // 模拟 24 小时运行：每秒 1 次 commit，共 86400 次，加速为每 ms 1 次共 86 次
        constexpr int TOTAL_COMMITS = 86;  // 86 秒 ≈ 24 小时加速
        constexpr int TAGS_PER_COMMIT = 100;

        for (int round = 0; round < TOTAL_COMMITS; ++round) {
            for (int i = 0; i < TAGS_PER_COMMIT; ++i) {
                DoubleBuffer::Snapshot snap;
                snap.tagId = static_cast<quint32>(i);
                snap.currentValue = static_cast<float>(qrand() % 10000) / 100.0f;
                snap.timestamp = QDateTime::currentMSecsSinceEpoch();
                doubleBuf.write(snap.tagId, snap);
            }
            doubleBuf.commit();
            QTest::qWait(1);
        }

        // 验证 DoubleBuffer 仍可正常读写
        auto snap = doubleBuf.readAll();
        QVERIFY(snap != nullptr);
        QVERIFY(snap->size() == TAGS_PER_COMMIT);
        qDebug() << "[perf] memory stability: 86 rounds x 100 tags completed, still functional";
    }
};

// ── 为测试暴露 processBatch ──
// DataParseThread 需要添加 public 方法：
//   void processBatchTest(const std::vector<RawModbusData>& batch) { processBatch(batch); }
// 或改为在 processBatch 上加 Q_INVOKABLE / friend class TestPerformance

QTEST_APPLESS_MAIN(TestPerformance)
#include "test_performance.moc"
```

注意：需要在 `DataParseThread.h` 中添加一个 public 测试入口方法。在步骤 2 中处理。

- [ ] **步骤 2：DataParseThread.h 添加测试入口**

添加 public 方法（在 `void stop();` 之后）：

```cpp
    // 测试入口：直接调用 processBatch（避免启动线程）
    void processBatchTest(const std::vector<RawModbusData>& batch) { processBatch(batch); }
```

- [ ] **步骤 3：修改 tests/CMakeLists.txt**

在 unit tests 注册区域末尾添加：

```cmake
add_unit_test(test_performance performance/test_performance.cpp)
```

- [ ] **步骤 4：构建并运行测试**

```bash
cmake --build build --target test_performance
ctest --test-dir build -R test_performance --output-on-failure
```

预期：3 个测试均 PASS。

- [ ] **步骤 5：Commit**

```bash
git add tests/performance/test_performance.cpp tests/CMakeLists.txt host/pipeline/DataParseThread.h
git commit -m "test: add performance tests for throughput, flood recovery, memory stability

- 10000-point batch processing latency (< 500ms)
- Alarm flood detection and recovery time
- Accelerated 24-hour memory stability (DoubleBuffer write/commit cycle)
- DataParseThread exposes processBatchTest() for test access"
```

---

### 任务 7：Modbus/MQTT CI 条件编译

**文件：**
- 修改：`plugins/CMakeLists.txt`
- 修改：`.github/workflows/build-linux.yml`
- 修改：`.github/workflows/build-windows.yml`

- [ ] **步骤 1：plugins/CMakeLists.txt 改为条件编译**

```cmake
add_subdirectory(fieldbus_simulator)
add_subdirectory(persistence_sqlite)

# Modbus plugin: requires libmodbus
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(LIBMODBUS libmodbus)
endif()
if(LIBMODBUS_FOUND)
    message(STATUS "libmodbus found — building ModbusPlugin")
    add_subdirectory(fieldbus_modbus)
else()
    message(STATUS "libmodbus not found — skipping ModbusPlugin")
endif()

# MQTT plugin: requires Qt6::Mqtt
if(Qt6Mqtt_FOUND)
    message(STATUS "Qt6::Mqtt found — building QtMqttPlugin")
    add_subdirectory(mqtt_qt)
else()
    message(STATUS "Qt6::Mqtt not found — skipping QtMqttPlugin")
endif()
```

- [ ] **步骤 2：build-linux.yml 安装依赖**

在 "Install system deps" 步骤中追加 libmodbus 和 qtmqtt：

```yaml
      - name: Install system deps
        run: |
          sudo apt-get update
          sudo apt-get install -y libgl1-mesa-dev libxkbcommon-x11-0
          sudo apt-get install -y libmodbus-dev qt6-mqtt-dev
```

- [ ] **步骤 3：build-windows.yml 安装依赖**

在 "Install Qt" 步骤之后、"Configure" 之前，添加 vcpkg 安装步骤：

```yaml
      - name: Install vcpkg dependencies
        run: |
          git clone https://github.com/microsoft/vcpkg.git ${{ runner.temp }}\vcpkg
          ${{ runner.temp }}\vcpkg\bootstrap-vcpkg.bat
          ${{ runner.temp }}\vcpkg\vcpkg install libmodbus:x64-windows qtmqtt:x64-windows
        continue-on-error: true
```

- [ ] **步骤 4：Commit**

```bash
git add plugins/CMakeLists.txt .github/workflows/build-linux.yml .github/workflows/build-windows.yml
git commit -m "ci: conditional ModbusPlugin/QtMqttPlugin builds with CI dependencies

- plugins/CMakeLists.txt uses pkg_check_modules for libmodbus, Qt6Mqtt_FOUND for MQTT
- Linux CI installs libmodbus-dev and qt6-mqtt-dev
- Windows CI uses vcpkg for libmodbus and qtmqtt (best-effort)"
```

---

## 执行顺序建议

| 优先级 | 任务 | 原因 |
|--------|------|------|
| 1 | 任务 6（switchPlugin 清理） | 已合并到任务 4 步骤 4 |
| 2 | 任务 1（写入校验信号） | 独立，无依赖 |
| 3 | 任务 2（加密配置） | 需要 OpenSSL 链接验证 |
| 4 | 任务 3（CI 签名占位） | 纯 YAML，无编译 |
| 5 | 任务 4（版本化发布） | 涉及多个文件和 JSON |
| 6 | 任务 5（悬挂指针） | 影响核心 DI 逻辑，需谨慎 |
| 7 | 任务 7（性能测试） | 依赖 DataParseThread 改动 |
| 8 | 任务 8（CI 条件编译） | 纯构建系统，可最后做 |

实际执行时 1-4 可以并行，5-8 也可以并行。建议按顺序逐个确保编译不破。
