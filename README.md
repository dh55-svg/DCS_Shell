# DCS_Shell — 插件化 DCS 上位机系统

基于 Qt6/C++17 的分布式控制系统（DCS）SCADA/HMI 上位机。通信协议全由插件管理，MVC 数据显示分离，支持运行时热插拔切换协议。

## 架构

```
plugin_interface/  ← 接口 SDK（宿主和插件共享，纯头文件）
       ↑
host/              ← 宿主 EXE（零通信实现代码）
  ├── domain/      → AlarmEngine (ISA-18.2) + TagManager
  ├── pipeline/    → DataParseThread → DoubleBuffer → HistorySampler
  ├── application/ → DataController / AlarmController
  ├── presentation/→ MainWindow + Models + Delegates (MVC)
  └── infrastructure/ → PluginHub (热插拔) + Nulls + Logger
       ↑ 运行时加载 (QPluginLoader)
plugins/
  ├── fieldbus_simulator/  → SimulatorPlugin.dll
  ├── fieldbus_modbus/     → ModbusPlugin.dll (需 libmodbus)
  ├── mqtt_qt/             → QtMqttPlugin.dll (需 Qt6::Mqtt)
  └── persistence_sqlite/  → SqlitePersistencePlugin.dll
```

## 特性

- **通信协议全插件管理**：宿主 EXE 不链接任何通信实现代码，运行时动态加载 DLL/SO
- **热插拔**：PluginHub 支持运行时 `switchPlugin()` / `unloadPlugin()` 切换协议
- **MVC 分离**：View → Controller → Domain 单向依赖，跨层仅用 Qt Signals & Slots
- **ISA-18.2 报警**：8 状态报警状态机 + 搁置/抑制/洪水检测/震荡保护 + KPI 监控
- **降级安全**：无插件时注入 Null 实现，系统正常启动不崩溃
- **跨平台**：Windows (.dll) + Linux (.so) 统一 CMake 构建

## 构建

### 依赖

- Qt 6.5+ (Core, Widgets, Sql, Network, SerialPort, Test)
- CMake 3.20+
- C++17 编译器 (GCC 11+ / MSVC 2022+ / Clang 14+)
- 可选：libmodbus (ModbusPlugin), Qt6::Mqtt (QtMqttPlugin)

### 构建命令

```bash
# 配置
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug

# 构建全部（宿主 + 插件 + 测试）
cmake --build build

# 仅构建宿主
cmake --build build --target DCS_Shell

# 构建插件
cmake --build build --target SimulatorPlugin SqlitePersistencePlugin
```

### 运行

```bash
# Windows (需要 Qt bin 在 PATH)
set PATH=C:\Qt\6.5.3\mingw_64\bin;%PATH%
build\host\DCS_Shell.exe

# Linux
export LD_LIBRARY_PATH=/opt/Qt/6.5.3/gcc_64/lib:$LD_LIBRARY_PATH
./build/host/DCS_Shell
```

## 测试

```bash
# 构建并运行全部测试
cmake --build build
ctest --test-dir build --output-on-failure

# 运行单个测试（带 XML 输出）
build\tests\test_alarm_state_machine.exe -o result.xml,xml
```

### 测试覆盖（15 单元测试 / 88 用例）

| 组件 | 测试 |
|------|------|
| AlarmStateMachine | 状态转移表 100% 覆盖 |
| DeadbandFilter | 死区触发/恢复 8 用例 |
| DeviationChecker | 偏差检测 4 用例 |
| RateOfChangeChecker | 变化率 4 用例 |
| DoubleBuffer | 读写分离 + 对象池 5 用例 |
| LockFreeRingBuffer | 无锁队列 7 用例 |
| TagManager | CRUD + 索引 + JSON 11 用例 |
| JsonConfigRepo | 配置读写 3 用例 |
| FloodDetector | 洪水检测 4 用例 |
| ChatteringGuard | 震荡保护 3 用例 |
| ShelveManager | 搁置管理 5 用例 |
| AppConfig | 配置解析 3 用例 |
| PluginHub | 插件中枢 6 用例 |
| SuppressionEngine | 条件抑制 7 用例 |
| AlarmEngine | 报警引擎 3 用例 |

## 配置

编辑 `config/app.json`：

```json
{
  "dbBackend": "sqlite",
  "fieldbus": "simulator",
  "mqtt": { "host": "127.0.0.1", "port": 1883, "enabled": false }
}
```

编辑 `config/tags.json` 配置位号（Modbus 地址 + 报警限值）。

## 许可证

MIT License
