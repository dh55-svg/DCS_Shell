// =============================================================================
// ModbusPlugin.h — Modbus 插件 (Q_PLUGIN_METADATA 包装)
// =============================================================================
#ifndef MODBUSPLUGIN_H
#define MODBUSPLUGIN_H
#include "plugin_interface/IFieldBus.h"
#include "ModbusImpl.h"

class ModbusPlugin : public ModbusImpl {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID IFieldBus_iid FILE "ModbusPlugin.json")
public:
    using ModbusImpl::ModbusImpl;
};
#endif
