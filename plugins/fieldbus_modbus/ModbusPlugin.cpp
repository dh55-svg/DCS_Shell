// =============================================================================
// ModbusPlugin.cpp — 插件入口 (空, 所有实现在基类 ModbusImpl 中)
// =============================================================================
#include "ModbusPlugin.h"
// Qt 标准模式：#include "moc_xxx.cpp" 触发 MOC 自动为 IFieldBus.h 生成元对象代码
#include "moc_IFieldBus.cpp"
// Q_PLUGIN_METADATA 宏由 MOC 生成, 不需要额外代码
