#ifndef APPCONTEXT_H
#define APPCONTEXT_H
#include <memory>
#include "plugin_interface/IFieldBus.h"
#include "plugin_interface/IAlarmRepo.h"
#include "plugin_interface/IHistoryRepo.h"
#include "plugin_interface/ITagRepo.h"
#include "plugin_interface/IOperationRepo.h"
#include "../domain/alarm/AlarmEngine.h"
#include "../domain/tag/tagmanager.h"
#include "../pipeline/DataPipeline.h"
#include "../infrastructure/logging/ILogger.h"
#include "../infrastructure/config/IConfigRepo.h"

struct AppContext {
    std::shared_ptr<AlarmEngine> alarmEngine;
    std::shared_ptr<TagManager> tagManager;
    std::shared_ptr<DataPipeline> dataPipeline;
    std::shared_ptr<IFieldbus> fieldbus;
    std::shared_ptr<IAlarmRepo> alarmRepo;
    std::shared_ptr<IHistoryRepo> historyRepo;
    std::shared_ptr<ITagRepo> tagRepo;
    std::shared_ptr<IOperationRepo> operationRepo;
    std::shared_ptr<ILogger> logger;
    std::shared_ptr<IConfigRepo> configRepo;
};
#endif
