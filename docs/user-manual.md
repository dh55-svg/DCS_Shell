# DCS_Shell 用户手册

> 插件化 DCS SCADA/HMI 上位机系统 · v1.0.0

---

## 目录

1. [系统概述](#1-系统概述)
2. [安装部署](#2-安装部署)
3. [配置说明](#3-配置说明)
4. [操作指南](#4-操作指南)
5. [插件系统](#5-插件系统)
6. [报警系统](#6-报警系统)
7. [安全机制](#7-安全机制)
8. [性能参考](#8-性能参考)
9. [故障排除](#9-故障排除)
10. [开发扩展](#10-开发扩展)

---

## 1 系统概述

### 1.1 架构总览

DCS_Shell 是一个基于 Qt6/C++17 的分布式控制系统上位机，采用纯插件化架构：

```
┌─────────────────────────────────────────────────────┐
│  Presentation (MVC)                                 │
│  MainWindow → Delegates → Models → ViewModels       │
├─────────────────────────────────────────────────────┤
│  Application Controllers                            │
│  DataController · AlarmController                   │
├─────────────────────────────────────────────────────┤
│  Domain                                             │
│  AlarmEngine (ISA-18.2) · TagManager               │
├─────────────────────────────────────────────────────┤
│  Pipeline                                           │
│  DataParseThread → DoubleBuffer → HistorySampler    │
├─────────────────────────────────────────────────────┤
│  Infrastructure                                     │
│  PluginHub · AuditLogger · Security · ILogger        │
├─────────────────────────────────────────────────────┤
│  Plugin Interface SDK (plugin_interface/)            │
│  IFieldbus · IAlarmRepo · IHistoryRepo · IMqttGateway│
├─────────────────────────────────────────────────────┤
│  Plugins (运行时加载)                                 │
│  Simulator · Modbus · Sqlite · MQTT                  │
└─────────────────────────────────────────────────────┘
```

### 1.2 核心特性

| 特性 | 说明 |
|------|------|
| **协议插件化** | 通信协议通过 QPluginLoader 动态加载，宿主零通信代码 |
| **热插拔** | 运行时 `switchPlugin()` 切换协议，系统不重启 |
| **ISA-18.2 报警** | 8 状态报警状态机 + 搁置/抑制/洪水检测/震荡保护 |
| **降级安全** | 无插件时注入 Null 实现，系统正常启动 |
| **操作审计** | 所有写操作/确认/屏蔽记录到独立审计日志 |
| **密码加密** | 配置文件密码 AES-256-CBC 加密存储 |
| **MQTT TLS** | MQTT 支持 TLS 加密传输 |

---

## 2 安装部署

### 2.1 系统要求

| 项目 | 要求 |
|------|------|
| 操作系统 | Windows 10/11 或 Ubuntu 22.04 |
| Qt | 6.5+ (Core, Widgets, Sql, Network, SerialPort) |
| CMake | 3.20+ |
| 编译器 | MSVC 2022+ / GCC 11+ / Clang 14+ |
| 磁盘 | 500 MB（含 Qt 运行时） |
| 内存 | 推荐 4 GB+ |

### 2.2 从源码构建

```bash
# 克隆
git clone <repo> DCS_Shell
cd DCS_Shell

# 配置
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

# 构建
cmake --build build --parallel

# 运行
# Windows: 确保 Qt bin 在 PATH 中
build/host/DCS_Shell.exe

# Linux:
export LD_LIBRARY_PATH=/opt/Qt/6.5.3/gcc_64/lib:$LD_LIBRARY_PATH
./build/host/DCS_Shell
```

### 2.3 Release 包安装

从 Releases 页面下载对应平台的压缩包：

**Windows 包结构：**
```
DCS_Shell/
├── DCS_Shell.exe
├── Qt6*.dll
├── platforms/         # Qt 插件
├── styles/            # Qt 样式
├── plugins/
│   ├── SimulatorPlugin.dll / .json
│   ├── ModbusPlugin.dll / .json
│   └── SqlitePersistencePlugin.dll / .json
├── config/
│   ├── app.json
│   └── tags.json
└── logs/
    ├── dcs_20250717.log
    └── audit_20250717.log
```

### 2.4 目录说明

| 路径 | 用途 |
|------|------|
| `config/app.json` | 应用配置（数据库、通信、MQTT） |
| `config/tags.json` | 位号配置（Modbus 地址、报警限值） |
| `logs/` | 运行日志（滚动按日切割） |
| `plugins/` | 插件 DLL/SO 及元数据 JSON |

---

## 3 配置说明

### 3.1 应用配置

```json
{
  "dbBackend": "sqlite",
  "mysql": {
    "host": "127.0.0.1",
    "port": 3306,
    "database": "dcs",
    "user": "root",
    "password": "ENC:xxx"
  },
  "sqlite": { "path": "data/dcs.db" },
  "fieldbus": "simulator",
  "mqtt": {
    "host": "127.0.0.1",
    "port": 1883,
    "enabled": false,
    "tls": {
      "enabled": false,
      "caCertPath": "./certs/ca.crt",
      "clientCertPath": "./certs/client.crt",
      "clientKeyPath": "./certs/client.key"
    }
  }
}
```

> **安全提示**：`mysql.password` 支持 `ENC:` 密文格式。使用工具生成：命令行传入明文，输出密文写入配置文件。

### 3.2 位号配置

`config/tags.json` 定义所有测量点（位号）及其报警参数：

```json
[
  {
    "tagId": 1001,
    "name": "P-101 压力",
    "modbusDeviceId": 1,
    "modbusServerAddr": 1,
    "modbusRegAddr": 0,
    "engLow": 0.0,
    "engHigh": 100.0,
    "unit": "MPa",
    "alarmEnabled": true,
    "highHighLimit": 95.0,
    "highLimit": 85.0,
    "lowLimit": 20.0,
    "lowLowLimit": 5.0,
    "deadband": 2.0,
    "rateOfChangeEnabled": true,
    "rateOfChangeLimit": 50.0,
    "deviationEnabled": true,
    "deviationLimit": 10.0
  }
]
```

---

## 4 操作指南

### 4.1 启动与连接

1. 启动 DCS_Shell.exe
2. 状态栏显示通信状态（绿色=在线，红色=离线）
3. 首次启动自动使用 Simulator 插件生成模拟数据

### 4.2 数据监控

- **实时值列表**：显示所有位号的当前值、质量戳、状态
- **趋势曲线**：选择位号查看历史趋势（最多 1800 点环形缓冲）
- **设备状态**：显示各 Modbus 从站的连接/通信状态

### 4.3 控制操作

| 操作 | 方法 |
|------|------|
| 写设定值 | 双击位号的 SP 列输入值 |
| 手/自动切换 | 点击位号的 MODE 列 |
| 输出值 | 在输出视图中编辑 |

> 所有写操作均经过 `Security::validateWriteValue()` 值域校验，范围在 `engLow ~ engHigh` 之间。超出范围的写入被拒绝并记录在审计日志中。

### 4.4 报警管理

| 操作 | 快捷键/交互 |
|------|-------------|
| 报警确认 | 右键 → 确认 或 点击确认按钮 |
| 搁置报警 | 右键 → 搁置 → 输入原因和时长 |
| 取消搁置 | 右键 → 恢复 |
| 抑制报警 | 配置抑制规则（按位号/条件） |
| 查看历史 | 报警列表支持按时间/级别筛选 |

---

## 5 插件系统

### 5.1 插件加载机制

系统启动时 `PluginHub::scanAll()` 扫描 `plugins/` 目录下的 `.dll`/`.so` 文件：
1. 读取 `.json` 元数据（name, version, priority）
2. 按 `priority` 排序
3. 最高优先级的插件通过 `QPluginLoader` 加载
4. 无有效插件时注入 Null 实现，系统不崩溃

### 5.2 热插拔操作

`PluginHub::switchPlugin(iid, newPluginPath)` 支持运行时切换：
1. 卸载当前插件（`unloadPlugin()`）
2. 清理 `m_discovered` 旧条目
3. 加载新插件并注册
4. 触发 `pluginSwitched` 信号通知上层

### 5.3 常见插件

| 插件 | 用途 | 依赖 |
|------|------|------|
| SimulatorPlugin | 开发测试用模拟数据源 | 无 |
| ModbusPlugin | Modbus RTU/TCP 通信 | libmodbus |
| SqlitePersistencePlugin | 报警/历史数据持久化 | Qt6::Sql |
| QtMqttPlugin | MQTT 网关（支持 TLS） | Qt6::Mqtt |

---

## 6 报警系统

### 6.1 ISA-18.2 状态机

```
Normal ──→ ActiveUnack ──→ Activeack ──→ ReturnToNormalunack ──→ ReturnToNormalack ──→ Normal
  │            │               │                                         │
  └──→ outOfService ←─────────┴──→ Shelved / SuppressedByDesign          │
                                                                         └──→ ActiveUnack (重新超限)
```

### 6.2 报警级别

| 级别 | 优先级 | 默认颜色 |
|------|--------|----------|
| 高-高 (HH) | Critical | 红色 |
| 高 (H) | Major | 橙色 |
| 低 (L) | Major | 黄色 |
| 低-低 (LL) | Critical | 深红 |
| 变化率 (ROC) | Major | 紫色 |
| 偏差 (Dev) | Major | 蓝色 |

### 6.3 高级特性

| 特性 | 说明 |
|------|------|
| On-Delay | 信号持续超限指定毫秒后才触发报警（3 秒默认） |
| Off-Delay | 信号恢复后持续正常指定毫秒才清除报警 |
| 死区 | `exceedsDeadbaud()` 防止阈值附近频繁触发 |
| 洪水检测 | 10 报警/10 分钟内自动抑制非紧急报警 |
| 震荡保护 | 检测震荡模式并限制报警频率 |
| KPI 监控 | 统计报警率、平均确认时间、平均恢复时间 |

---

## 7 安全机制

### 7.1 密码加密

`CryptoConfig.h` 提供 SHA-256 派生密钥的 AES-256-CBC 加密：
- `encryptPassword()` → `ENC:<base64_iv>:<base64_ciphertext>`
- `decryptPassword()` — 自动解密 `ENC:` 前缀密文
- 无需外部 OpenSSL 依赖，纯 Qt 实现

### 7.2 操作审计

所有关键操作记录到 `logs/audit_YYYYMMDD.log`：

| 操作 | 触发布局 | 示例 |
|------|----------|------|
| write | DataPipeline::writeSetPoint | `[时间] operator \| write \| tagId=1001 \| setPoint=75.5` |
| mode | DataPipeline::setAutoMode | `[时间] operator \| mode \| tagId=1001 \| auto` |
| ack | AlarmEngine::acknowledgeAlarm | `[时间] operator \| ack \| ALM-001 \| ` |
| shelve | AlarmEngine::shelveAlarm | `[时间] operator \| shelve \| tagId=1001 \| 临时屏蔽 (3600s)` |
| switch | PluginHub::switchPlugin | `[时间] operator \| switch \| IFieldBus \| /path/to/new.so` |

### 7.3 值域校验

所有写入操作执行 `Security::validateWriteValue(value, engLow, engHigh)`，越界写入被拒绝并触发 `writeRejected` 信号。

### 7.4 网络安全

- **MQTT 加密**：`IMqttGateway.connectEncrypted()` 支持 TLS I：带 CA 证书/客户端证书/私钥
- **配置**：`app.json` → `mqtt.tls` 部分配置
- **Modbus**：推荐在网络层使用 VPN 或 stunnel 隧道加密

---

## 8 性能参考

在以下环境测试：
- CPU: Intel Core i7-12700
- 内存: 32 GB DDR5
- 操作系统: Windows 11 / Ubuntu 22.04

| 场景 | 指标 | 值 |
|------|------|----|
| 位号处理吞吐量 | 10000 位号批量处理 | < 200 ms |
| 报警注入 | 500 报警/秒 | < 100 ms (FloodDetector) |
| DoubleBuffer 提交 | 100 位号/轮 | < 10 ms |
| 长期稳定性 | 10000 轮 × 100 位号 | 无内存泄漏 |

---

## 9 故障排除

### 9.1 启动问题

| 现象 | 原因 | 解决 |
|------|------|------|
| "missing critical dependency" | 无任何 fieldbus 插件 | 确认 plugins/ 目录存在且至少有一个插件 DLL |
| `Cannot open config file` | config/ 目录未找到 | 从工作目录启动，或使用 `findConfigDir()` 的候选路径 |
| OpenSSL 错误 | 本地无 OpenSSL | CryptoConfig.h 纯 Qt 实现，无需外部 OpenSSL |

### 9.2 运行问题

| 现象 | 排查 |
|------|------|
| 所有位号 Quality=Bad | Fieldbus 插件未启动或通信断开 |
| 报警不触发 | 检查 `alarmEnabled` 配置、deadband 和 On-Delay 设置 |
| 数据库写入慢 | SQLite 使用 WAL 模式，检查磁盘 IO |
| 审计日志文件增大 | 日志按日切割，建议配置外部日志轮转工具 |

### 9.3 调试模式

使用 Debug 构建可获取详细日志输出：
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
./build/host/DCS_Shell.exe
```

日志文件位于 `logs/dcs_YYYYMMDD.log`。

---

## 10 开发扩展

### 10.1 新增通信插件

1. 实现 `IFieldbus` 接口（继承 `IFieldbus`，实现纯虚函数）
2. 导出插件宏 `Q_PLUGIN_METADATA` + `Q_INTERFACES`
3. 创建 `.json` 元数据文件（name/version/author/compatibility/priority）
4. 在 `plugins/CMakeLists.txt` 注册

### 10.2 编译规则

```cmake
# plugins/my_protocol/CMakeLists.txt
add_library(MyProtocolPlugin MODULE
    MyProtocolPlugin.cpp MyProtocolImpl.cpp
)
target_link_libraries(MyProtocolPlugin PRIVATE
    plugin_interface Qt6::Core Qt6::Network
)
install(TARGETS MyProtocolPlugin DESTINATION plugins/)
```

### 10.3 API 文档

Doxygen 文档通过 CI 自动生成，可在每次构建的 Artifacts 中找到：
- `docs/api/index.html` — HTML 文档
- `docs/api/xml/` — XML 原始数据

本地生成：
```bash
doxygen Doxyfile
# 输出: docs/api/index.html
```

---

> **文档版本**: v1.0.0 · 最后更新: 2025-07-17
