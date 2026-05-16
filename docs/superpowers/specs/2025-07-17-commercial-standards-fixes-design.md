# 商业标准修复设计文档

**日期：** 2025-07-17
**版本：** 1.0.0
**状态：** 已批准

## 概述

基于商业标准审查报告的 8 项修复，覆盖代码缺陷、工程化、质量保障三个维度。

---

## 模块一：Modbus 写入值域校验

**问题：** `Security::validateWriteValue()` 已定义但无调用者。写路径缺少值域校验，存在越界写入风险。

**设计：**
- 在 `DataPipeline::writeSetPoint()` 入口处调用 `Security::validateWriteValue()`
- 校验不通过时 emit `writeRejected(quint32 tagId, float value, QString reason)` 信号并 return
- 新增信号声明于 `DataPipeline.h`

**文件：** `host/pipeline/DataPipeline.h`, `host/pipeline/DataPipeline.cpp`

---

## 模块二：MySQL 密码 AES-256 加密

**问题：** `config/app.json` 中 `mysql.password` 明文存储。

**设计（选型 A）：**
- 新建 `host/infrastructure/security/CryptoConfig.h`：提供 `encryptPassword()` / `decryptPassword()`，使用 AES-256-CBC，密钥硬编码为编译时常量
- 密文以 `ENC:` 前缀存储于 JSON 中
- `AppConfig::fromJson()` 加载时自动检测并解密
- 可选：提供 `tools/encrypt_password.cpp` 命令行工具生成密文

**文件：** 新建 `CryptoConfig.h`，修改 `host/app/AppConfig.h`

---

## 模块三：Windows 签名 CI 占位

**问题：** CI 无 Authenticode 签名步骤。

**设计（选型 A）：**
- `build-windows.yml` 中添加注释掉的签名步骤
- 模板包含 `signtool sign` 完整命令 + 所需 GitHub Secrets 说明
- 获取正式证书后取消注释并配置 Secrets 即可启用

**文件：** `.github/workflows/build-windows.yml`

---

## 模块四：版本化发布流程

**问题：** 无 CHANGELOG、无版本标签、插件元数据仅含 priority。

**设计：**
- 新建 `CHANGELOG.md`，记录 v1.0.0
- 插件 `.json` 扩展字段：`name`, `version`, `author`, `compatibility`
- `PluginDescriptor` 结构体增加对应字段
- `PluginHub::scanAll()` 解析新字段

**文件：** 新建 `CHANGELOG.md`，修改 `host/infrastructure/plugin/PluginDescriptor.h`（或 `PluginHub.h`），修改 4 个 `.json` 文件

---

## 模块五：悬挂指针修复

**问题：** `ApplicationBuilder::withFieldbus()` 使用空删除器的 `shared_ptr`。`PluginHub` 裸指针若先于 `AppContext` 析构，`IFieldbus*` 悬挂。

**设计：**
- `AppContext` 中 `PluginHub*` 改为 `std::shared_ptr<PluginHub>`
- `ApplicationBuilder::withPluginHub()` 接收 `shared_ptr`
- `withFieldbus()` / `withDatabase()` 使用 `std::shared_ptr` aliasing constructor：`std::shared_ptr<IFieldbus>(m_pluginHub, fb)` —— 共享 PluginHub 所有权，确保 IFieldbus 不会在 PluginHub 销毁后访问
- `main.cpp` 中 PluginHub 改为 `make_shared`

**文件：** `host/app/AppContext.h`, `host/app/ApplicationBuilder.h`, `host/main.cpp`

---

## 模块六：switchPlugin 清理旧记录

**问题：** `PluginHub::switchPlugin()` 追加新 descriptor 但不删除 m_discovered 中旧条目，`availablePlugins()` 返回过时路径。

**设计：**
- `switchPlugin()` 中追加前先遍历 `m_discovered`，移除所有匹配 iid 的旧条目

**文件：** `host/infrastructure/plugin/PluginHub.cpp`

---

## 模块七：轻量性能测试

**问题：** 无性能/负载测试。

**设计（选型 A）：**
- 新建 `tests/performance/test_performance.cpp`，3 个测试用例：
  1. `large_point_throughput`：10000 位号批量处理延迟 < 500ms
  2. `alarm_flood_recovery`：500 报警/秒洪水检测 + 恢复时间 < 5s
  3. `memory_stability`：加速 24 小时循环，内存增长 < 10MB
- 集成到 `ctest`，每个用例 30 秒内完成

**文件：** 新建 `tests/performance/test_performance.cpp`，修改 `tests/CMakeLists.txt`

---

## 模块八：Modbus/MQTT CI 条件编译

**问题：** `fieldbus_modbus` 和 `mqtt_qt` 在 CI 中被注释，从未编译测试。

**设计（选型 C）：**
- `plugins/CMakeLists.txt` 使用 `pkg_check_modules` 检测 libmodbus，`Qt6Mqtt_FOUND` 检测 MQTT
- Linux CI：`apt install libmodbus-dev qt6-mqtt-dev`
- Windows CI：`vcpkg install libmodbus qt6-mqtt`

**文件：** `plugins/CMakeLists.txt`, `.github/workflows/build-linux.yml`, `.github/workflows/build-windows.yml`
