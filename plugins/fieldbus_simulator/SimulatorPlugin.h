// =============================================================================
// SimulatorPlugin.h
// =============================================================================
#ifndef SIMULATORPLUGIN_H
#define SIMULATORPLUGIN_H
#include "plugin_interface/IFieldBus.h"
#include "SimulatorImpl.h"

class SimulatorPlugin : public SimulatorImpl {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID IFieldBus_iid FILE "SimulatorPlugin.json")
public:
    using SimulatorImpl::SimulatorImpl;
};
#endif
