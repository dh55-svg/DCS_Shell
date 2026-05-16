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
