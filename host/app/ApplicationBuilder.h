#ifndef APPLICATIONBUILDER_H
#define APPLICATIONBUILDER_H
#include "AppContext.h"
#include "AppConfig.h"
#include "plugin_interface/IFieldBus.h"
#include "plugin_interface/IAlarmRepo.h"
#include "plugin_interface/IHistoryRepo.h"
#include "plugin_interface/IMqttGateway.h"
#include "../infrastructure/plugin/PluginHub.h"
#include "../infrastructure/nulls/NullFieldBus.h"
#include "../infrastructure/nulls/NullAlarmRepo.h"
#include "../infrastructure/nulls/NullHistoryRepo.h"
#include "../infrastructure/nulls/NullTagRepo.h"
#include "../infrastructure/nulls/NullOperationRepo.h"
#include "../infrastructure/logging/spdlogadapter.h"
#include "../infrastructure/config/JsonConfigRepo.h"
#include "../domain/tag/tagmanager.h"
#include "../domain/alarm/AlarmEngine.h"
#include "../pipeline/DataPipeline.h"
#include <memory>

class ApplicationBuilder {
public:
    ApplicationBuilder() { m_ctx = std::make_shared<AppContext>(); }

    ApplicationBuilder& withConfig(const AppConfig& cfg) { m_cfg = cfg; return *this; }
    ApplicationBuilder& withPluginHub(std::shared_ptr<PluginHub> hub) {
        m_pluginHub = hub;
        m_ctx->pluginHub = hub;
        return *this;
    }

    ApplicationBuilder& withLogger() {
        auto logger = std::make_shared<FileLogger>();
        logger->setLogDir("./logs");
        m_ctx->logger = logger;
        return *this;
    }

    ApplicationBuilder& withConfigRepo() {
        m_ctx->configRepo = std::make_shared<JsonConfigRepo>();
        return *this;
    }

    ApplicationBuilder& withFieldbus() {
        if (m_pluginHub) {
            IFieldbus* fb = m_pluginHub->resolve<IFieldbus>(IFieldBus_iid);
            if (fb) {
                // aliasing shared_ptr: 共享 PluginHub 所有权，指向 IFieldbus
                m_ctx->fieldbus = std::shared_ptr<IFieldbus>(m_pluginHub, fb);
                return *this;
            }
        }
        m_ctx->fieldbus = std::make_shared<NullFieldbus>();
        return *this;
    }

    ApplicationBuilder& withDatabase() {
        if (m_pluginHub) {
            IAlarmRepo* alarm = m_pluginHub->resolve<IAlarmRepo>(IAlarmRepo_iid);
            IHistoryRepo* history = m_pluginHub->resolve<IHistoryRepo>(IHistoryRepo_iid);
            if (alarm && history) {
                // aliasing shared_ptr: 共享 PluginHub 所有权
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

    ApplicationBuilder& withDomain() {
        auto* logger = m_ctx->logger.get();
        m_ctx->tagManager = std::make_shared<TagManager>(*m_ctx->tagRepo, logger);
        m_ctx->alarmEngine = std::make_shared<AlarmEngine>(*m_ctx->alarmRepo, m_ctx->tagManager.get(), logger);
        m_ctx->alarmEngine->initialize();
        return *this;
    }

    ApplicationBuilder& withPipeline() {
        m_ctx->dataPipeline = std::make_shared<DataPipeline>();
        m_ctx->dataPipeline->setTagManager(m_ctx->tagManager.get());
        m_ctx->dataPipeline->setAlarmEngine(m_ctx->alarmEngine.get());
        m_ctx->dataPipeline->setFieldbus(m_ctx->fieldbus.get());
        m_ctx->dataPipeline->setHistoryRepo(m_ctx->historyRepo.get());
        m_ctx->dataPipeline->setLogger(m_ctx->logger.get());
        return *this;
    }

    std::shared_ptr<AppContext> build() { return m_ctx; }

private:
    std::shared_ptr<AppContext> m_ctx;
    AppConfig m_cfg;
    std::shared_ptr<PluginHub> m_pluginHub;
};
#endif
